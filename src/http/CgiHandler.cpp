#include "../../includes/http/CgiHandler.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>

#include <iostream>
#include <ctime>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <cstdio>


CgiHandler::CgiHandler() {}

CgiHandler::~CgiHandler() {}

CgiHandler::CgiHandler(const CgiHandler &other)
{
	(void)other;
}

CgiHandler &CgiHandler::operator=(const CgiHandler &other)
{
	(void)other;
	return *this;
}

std::string CgiHandler::_interpreterFor(const std::string& scriptPath) const
{
	std::string::size_type dotPos = scriptPath.find_last_of('.');
	if (dotPos == std::string::npos)
		return "";

	const std::string ext = scriptPath.substr(dotPos + 1);
	if (ext == "py")
		return "/usr/bin/python3";
	if (ext == "php")
		return "/usr/bin/php-cgi";
	if (ext == "sh")
		return "/bin/sh";
	return "";
}


std::vector<std::string> CgiHandler::_buildEnvp(const Request& req, const std::string& scriptPath)
{
    std::vector<std::string> env;

    // 1. Limpamos o PATH (Remove o \r se existir)
    std::string cleanPath = req.getPath();
    if (!cleanPath.empty() && cleanPath[cleanPath.length() - 1] == '\r')
        cleanPath.erase(cleanPath.length() - 1);

    // 2. Limpamos a Query String
    std::string cleanQuery = req.getQueryString();
    if (!cleanQuery.empty() && cleanQuery[cleanQuery.length() - 1] == '\r')
        cleanQuery.erase(cleanQuery.length() - 1);

    // 3. Limpamos os Headers críticos
    std::string cType = req.getHeader("Content-Type");
    if (!cType.empty() && cType[cType.length() - 1] == '\r')
        cType.erase(cType.length() - 1);

    // Montando o Env Oficial
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("REQUEST_METHOD=" + req.getMethod());
    
    // O Mundo da URL (Limpo)
    env.push_back("SCRIPT_NAME=" + cleanPath);
    env.push_back("PATH_INFO=" + cleanPath);
    env.push_back("REQUEST_URI=" + cleanPath); // Variável que o tester adora
    
    // O Mundo do Disco
    env.push_back("PATH_TRANSLATED=" + scriptPath);
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    
    // O Payload
    env.push_back("QUERY_STRING=" + cleanQuery);
    env.push_back("CONTENT_TYPE=" + cType);
    
    std::ostringstream ss;
    ss << req.getBody().length();
    env.push_back("CONTENT_LENGTH=" + ss.str());
    
    env.push_back("SERVER_PORT=9080");

    // =====================================================================
    // 4. INJETANDO OS "SPECIAL HEADERS" DO CLIENTE NO MODO CGI
    // =====================================================================
    const std::map<std::string, std::string>& headers = req.getHeaders();
    
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string headerName = it->first;
        std::string headerValue = it->second;

        // Limpa o \r do valor, por garantia
        if (!headerValue.empty() && headerValue[headerValue.length() - 1] == '\r')
            headerValue.erase(headerValue.length() - 1);

        // Formata a chave para o padrão CGI: prefixo HTTP_, maiúsculas, '-' vira '_'
        std::string envKey = "HTTP_";
        for (size_t i = 0; i < headerName.length(); ++i) {
            if (headerName[i] == '-') {
                envKey += '_';
            } else {
                envKey += std::toupper(headerName[i]);
            }
        }
        
        env.push_back(envKey + "=" + headerValue);
    }

    return env;
}


// Rewrites a path so it still works from inside dir (the CGI runs chdir'ed
// there). getcwd/realpath are not allowed, so a relative path just climbs one
// "../" per component of dir. Returns false when that is not possible.
bool CgiHandler::_pathFromDir(const std::string& dir, const std::string& path, std::string& out) const
{
	if (!path.empty() && path[0] == '/') {
		out = path;
		return true;
	}
	if (dir.empty() || dir[0] == '/')
		return false;

	std::string ups;
	std::string::size_type start = 0;
	while (start <= dir.size()) {
		std::string::size_type end = dir.find('/', start);
		if (end == std::string::npos)
			end = dir.size();
		const std::string part = dir.substr(start, end - start);
		if (part == "..")
			return false;
		if (!part.empty() && part != ".")
			ups += "../";
		start = end + 1;
	}
	out = ups + path;
	return true;
}

// scriptPath is the requested file on disk. interpreter is the location's
// cgi_pass when there is one, otherwise it is picked by extension (py/php/sh),
// and with neither the script is executed by itself.
CgiInfo CgiHandler::startCgi(const Request& req, const std::string& scriptPath, const std::string& interpreter)
{
    // Everything the child needs is prepared before fork(). The subject wants
    // the CGI to run in the script's directory, so there are two versions of
    // the paths: relative to that directory (chdir worked) and the original ones.
    std::string execPath = interpreter.empty() ? _interpreterFor(scriptPath) : interpreter;
    const bool scriptIsExec = execPath.empty();
    if (scriptIsExec)
        execPath = scriptPath;

    std::string runDir, execInDir, scriptInDir;
    const std::string::size_type slash = scriptPath.find_last_of('/');
    if (slash != std::string::npos && slash > 0) {
        const std::string dir = scriptPath.substr(0, slash);
        if (_pathFromDir(dir, execPath, execInDir)) {
            runDir = dir;
            scriptInDir = scriptPath.substr(slash + 1);
            if (scriptIsExec)
                execInDir = "./" + scriptInDir;
        }
    }

    int outPipe[2];
    if (pipe(outPipe) < 0)
        throw std::runtime_error("cgi: pipe failed");

    // =========================================================================
    // 1. CRIANDO O ARQUIVO TEMPORÁRIO ÚNICO PARA CADA REQUISIÇÃO (Anti-Colisão)
    // =========================================================================
    static int cgi_file_counter = 0;
    std::ostringstream ss;
    ss << "/tmp/webserv_cgi_body_" << cgi_file_counter++ << ".tmp";
    std::string tmp_name = ss.str();

    int tmpFd = open(tmp_name.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (tmpFd < 0)
        throw std::runtime_error("cgi: tmp file failed");

    // Removemos o nome do arquivo da pasta /tmp IMEDIATAMENTE.
    // O arquivo continuará existindo invisível no HD até que todos os FDs 
    // (pai e filho) o fechem. Isso garante isolamento total e disco sempre limpo.
    std::remove(tmp_name.c_str());
    // =========================================================================

    // Escrevemos os 100MB no disco de uma vez (o SO lida com isso perfeitamente)
    const std::string& body = req.getBody();
    if (req.getMethod() == "POST" && !body.empty()) {
        size_t written = 0;
        while (written < body.size()) {
            ssize_t n = write(tmpFd, body.data() + written, body.size() - written);
            if (n <= 0)
                break;
            written += n;
        }
        lseek(tmpFd, 0, SEEK_SET); // Volta o "cursor" para o início do arquivo
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(tmpFd);
        close(outPipe[0]);
        close(outPipe[1]);
        throw std::runtime_error("cgi: fork failed");
    }

    if (pid == 0) // Processo filho: vira o CGI
    {
        // Conecta a entrada padrão (STDIN) ao nosso arquivo temporário único
        dup2(tmpFd, STDIN_FILENO);

        // Conecta a saída (STDOUT) ao tubo de envio
        dup2(outPipe[1], STDOUT_FILENO);

        // The child inherits every socket and pipe the server has open. Holding
        // them would keep other clients' connections (and other scripts' pipes)
        // alive until this script exits, so drop everything but stdin/out/err.
        for (int fd = 3; fd < CGI_MAX_INHERITED_FD; ++fd)
            close(fd);

        // Roda no diretório do script (caminhos relativos do CGI funcionam).
        // Se o chdir falhar, segue com os caminhos originais.
        const bool moved = !runDir.empty() && chdir(runDir.c_str()) == 0;
        const std::string& exe = moved ? execInDir : execPath;
        const std::string& script = moved ? scriptInDir : scriptPath;

        char *argv[3];
        argv[0] = const_cast<char*>(exe.c_str());
        argv[1] = scriptIsExec ? NULL : const_cast<char*>(script.c_str());
        argv[2] = NULL;

        std::vector<std::string> envStr = _buildEnvp(req, script);
        std::vector<char*> envp;
        for (size_t i = 0; i < envStr.size(); ++i)
            envp.push_back(const_cast<char*>(envStr[i].c_str()));
        envp.push_back(NULL);

        execve(exe.c_str(), argv, &envp[0]);
        
        perror("ERRO NO EXECVE DO CGI");
        _exit(1);
    }

    // Processo pai (o servidor)
    close(tmpFd);      // Fecha o arquivo no pai (ele só ficará vivo agora no filho!)
    close(outPipe[1]); // Fecha a escrita do tubo de saída

    int flags = fcntl(outPipe[0], F_GETFL, 0);
    if (flags >= 0)
        fcntl(outPipe[0], F_SETFL, flags | O_NONBLOCK);

    CgiInfo info;
    info.readFd = outPipe[0];
    info.pid = pid;
    info.start = std::time(NULL);

    return info;
}

