#include "../../includes/http/CgiHandler.hpp"
#include "../../includes/http/Request.hpp"

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

std::string CgiHandler::execute(const Request& req, const std::string& scriptPath)
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

		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);

		const std::string interpreter = _interpreterFor(scriptPath);
		char *argv[3];

		if (!interpreter.empty())
		{
			argv[0] = const_cast<char*>(interpreter.c_str());
			argv[1] = const_cast<char*>(scriptPath.c_str());
			argv[2] = NULL;
		}
		else
		{
			argv[0] = const_cast<char*>(scriptPath.c_str());
			argv[1] = NULL;
			argv[2] = NULL;
		}

		std::vector<std::string> envStr = _buildEnvp(req, scriptPath);
		std::vector<char*> envp;
		for (size_t i = 0; i < envStr.size(); ++i)
			envp.push_back(const_cast<char*>(envStr[i].c_str()));
		envp.push_back(NULL);

		const char* execPath = interpreter.empty() ? scriptPath.c_str() : interpreter.c_str();
		execve(execPath, argv, &envp[0]);
		_exit(1);
	}

	close(inPipe[0]);
	close(outPipe[1]);

	if (req.getMethod() == "POST" && !req.getBody().empty())
	{
		const std::string& body = req.getBody();
		ssize_t sent = 0;
		while (sent < static_cast<ssize_t>(body.size()))
		{
			const ssize_t wrote = write(inPipe[1], body.c_str() + sent, body.size() - static_cast<size_t>(sent));
			if (wrote <= 0)
				break;
			sent += wrote;
		}
	}
	close(inPipe[1]);

	int flags = fcntl(outPipe[0], F_GETFL, 0);
	if (flags >= 0)
		fcntl(outPipe[0], F_SETFL, flags | O_NONBLOCK);

	std::string output;
	char buffer[4096];
	std::time_t start = std::time(NULL);
	while (true)
	{
		int status = 0;
		pid_t done = waitpid(pid, &status, WNOHANG);
		if (done < 0)
		{
			close(outPipe[0]);
			throw std::runtime_error("cgi: waitpid failed");
		}
		if (done == pid)
		{
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
			{
				close(outPipe[0]);
				throw std::runtime_error("cgi: script execution failed");
			}
			break;
		}

		pollfd pfd;
		pfd.fd = outPipe[0];
		pfd.events = POLLIN;
		pfd.revents = 0;

		int pollRes = poll(&pfd, 1, 100);
		if (pollRes > 0 && (pfd.revents & POLLIN))
		{
			ssize_t bytes = read(outPipe[0], buffer, sizeof(buffer));
			if (bytes > 0)
				output.append(buffer, static_cast<std::string::size_type>(bytes));
		}

		if (std::time(NULL) - start >= 5)
		{
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			close(outPipe[0]);
			throw std::runtime_error("cgi: execution timeout");
		}
	}

	while (true)
	{
		ssize_t bytes = read(outPipe[0], buffer, sizeof(buffer));
		if (bytes <= 0)
			break;
		output.append(buffer, static_cast<std::string::size_type>(bytes));
	}

	close(outPipe[0]);
	return output;
}

