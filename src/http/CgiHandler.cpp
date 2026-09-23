#include "../../includes/http/CgiHandler.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>

#include <ctime>
#include <stdexcept>
#include <vector>

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

	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env.push_back("REQUEST_METHOD=" + req.getMethod());
	env.push_back("SCRIPT_FILENAME=" + scriptPath);
	env.push_back("SCRIPT_NAME=" + req.getPath());
	env.push_back("QUERY_STRING=" + req.getQueryString());
	env.push_back("CONTENT_TYPE=" + req.getHeader("Content-Type"));
	env.push_back("CONTENT_LENGTH=" + req.getHeader("Content-Length"));
	env.push_back("REDIRECT_STATUS=200");
	return env;
}

CgiInfo CgiHandler::startCgi(const Request& req, const std::string& scriptPath)
{
	int inPipe[2];
	int outPipe[2];

	if (pipe(inPipe) < 0)
		throw std::runtime_error("cgi: pipe failed");
	if (pipe(outPipe) < 0)
	{
		close(inPipe[0]);
		close(inPipe[1]);
		throw std::runtime_error("cgi: pipe failed");
	}

	const pid_t pid = fork();
	if (pid < 0)
	{
		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);
		throw std::runtime_error("cgi: fork failed");
	}

	if (pid == 0)
	{
		if (dup2(inPipe[0], STDIN_FILENO) < 0 || dup2(outPipe[1], STDOUT_FILENO) < 0)
			_exit(1);

		// The env must be built before chdir(): it carries the path as the
		// server sees it, not as the script directory sees it.
		std::vector<std::string> envStr = _buildEnvp(req, scriptPath);
		std::vector<char*> envp;
		for (size_t i = 0; i < envStr.size(); ++i)
			envp.push_back(const_cast<char*>(envStr[i].c_str()));
		envp.push_back(NULL);

		// Drop every descriptor inherited from the server: listening sockets,
		// other clients and, above all, the pipes of other running CGIs. Keeping
		// another script's stdin write end open would stop it from ever seeing EOF.
		for (int fd = 3; fd < 1024; ++fd)
			close(fd);

		// Run from the script directory so relative paths inside it resolve.
		std::string scriptName = scriptPath;
		const std::string::size_type slash = scriptPath.find_last_of('/');
		if (slash != std::string::npos)
		{
			const std::string dir = scriptPath.substr(0, slash);
			if (chdir(dir.c_str()) < 0)
				_exit(1);
			scriptName = "./" + scriptPath.substr(slash + 1);
		}

		const std::string interpreter = _interpreterFor(scriptPath);
		char *argv[3];

		if (!interpreter.empty())
		{
			argv[0] = const_cast<char*>(interpreter.c_str());
			argv[1] = const_cast<char*>(scriptName.c_str());
			argv[2] = NULL;
		}
		else
		{
			argv[0] = const_cast<char*>(scriptName.c_str());
			argv[1] = NULL;
			argv[2] = NULL;
		}

		const char* execPath = interpreter.empty() ? scriptName.c_str() : interpreter.c_str();
		execve(execPath, argv, &envp[0]);
		_exit(1);
	}

	close(inPipe[0]);
	close(outPipe[1]);

	int flags = fcntl(outPipe[0], F_GETFL, 0);
	if (flags >= 0)
		fcntl(outPipe[0], F_SETFL, flags | O_NONBLOCK);

	CgiInfo info;
	info.readFd = outPipe[0];
	info.pid = pid;
	info.start = std::time(NULL);

	// The body is fed to the script by the main poll() loop. Writing it here
	// would block the whole server as soon as it outgrows the pipe buffer.
	if (req.getMethod() == "POST" && !req.getBody().empty())
	{
		flags = fcntl(inPipe[1], F_GETFL, 0);
		if (flags >= 0)
			fcntl(inPipe[1], F_SETFL, flags | O_NONBLOCK);
		info.writeFd = inPipe[1];
	}
	else
		close(inPipe[1]); // nothing to send: let the script see EOF right away

	return info;
}

