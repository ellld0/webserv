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

    std::string cleanPath = req.getPath();
    if (!cleanPath.empty() && cleanPath[cleanPath.length() - 1] == '\r')
        cleanPath.erase(cleanPath.length() - 1);

    std::string cleanQuery = req.getQueryString();
    if (!cleanQuery.empty() && cleanQuery[cleanQuery.length() - 1] == '\r')
        cleanQuery.erase(cleanQuery.length() - 1);

    std::string cType = req.getHeader("Content-Type");
    if (!cType.empty() && cType[cType.length() - 1] == '\r')
        cType.erase(cType.length() - 1);

    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("REQUEST_METHOD=" + req.getMethod());
    
    env.push_back("SCRIPT_NAME=" + cleanPath);
    env.push_back("PATH_INFO=" + cleanPath);
    env.push_back("REQUEST_URI=" + cleanPath);
    
    env.push_back("PATH_TRANSLATED=" + scriptPath);
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    
    env.push_back("QUERY_STRING=" + cleanQuery);
    env.push_back("CONTENT_TYPE=" + cType);
    
    std::ostringstream ss;
    ss << req.getBody().length();
    env.push_back("CONTENT_LENGTH=" + ss.str());
    
    env.push_back("SERVER_PORT=9080");

    const std::map<std::string, std::string>& headers = req.getHeaders();
    
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string headerName = it->first;
        std::string headerValue = it->second;

        if (!headerValue.empty() && headerValue[headerValue.length() - 1] == '\r')
            headerValue.erase(headerValue.length() - 1);

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

CgiInfo CgiHandler::startCgi(const Request& req, const std::string& scriptPath, const std::string& interpreter)
{
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

    static int cgi_file_counter = 0;
    std::ostringstream ss;
    ss << "/tmp/webserv_cgi_body_" << cgi_file_counter++ << ".tmp";
    std::string tmp_name = ss.str();

    int tmpFd = open(tmp_name.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (tmpFd < 0)
        throw std::runtime_error("cgi: tmp file failed");

    const std::string& body = req.getBody();
    if (req.getMethod() == "POST" && !body.empty()) {
        size_t written = 0;
        while (written < body.size()) {
            ssize_t n = write(tmpFd, body.data() + written, body.size() - written);
            if (n <= 0)
                break;
            written += n;
        }
        int readFd = open(tmp_name.c_str(), O_RDONLY);
        if (readFd < 0) {
            close(tmpFd);
            throw std::runtime_error("cgi: input file reopen failed");
        }
        close(tmpFd);
        tmpFd = readFd;
    }
    std::remove(tmp_name.c_str());

    const pid_t pid = fork();
    if (pid < 0) {
        close(tmpFd);
        close(outPipe[0]);
        close(outPipe[1]);
        throw std::runtime_error("cgi: fork failed");
    }

    if (pid == 0)
    {
        dup2(tmpFd, STDIN_FILENO);

        dup2(outPipe[1], STDOUT_FILENO);

        for (int fd = 3; fd < CGI_MAX_INHERITED_FD; ++fd)
            close(fd);

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
        
        throw std::runtime_error("cgi: execve failed");
    }

    close(tmpFd);
    close(outPipe[1]);

    int flags = fcntl(outPipe[0], F_GETFL, 0);
    if (flags >= 0)
        fcntl(outPipe[0], F_SETFL, flags | O_NONBLOCK);

    CgiInfo info;
    info.readFd = outPipe[0];
    info.pid = pid;
    info.start = std::time(NULL);

    return info;
}

