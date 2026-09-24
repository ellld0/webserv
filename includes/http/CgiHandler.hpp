#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <vector>
#include <ctime>
#include <sys/types.h>
#include "../../includes/http/Request.hpp"

// Upper bound of fds the CGI child closes before execve (the default ulimit)
#define CGI_MAX_INHERITED_FD 1024

struct CgiInfo {
    int         readFd;     // parent reads the script output here
    int         writeFd;    // parent writes the request body here (-1 when there is none)
    pid_t       pid;
    std::time_t start;

    CgiInfo() : readFd(-1), writeFd(-1), pid(-1), start(0) {}
};

class CgiHandler {
public:
    CgiHandler();
    ~CgiHandler();
    CgiHandler(const CgiHandler &other);
    CgiHandler &operator=(const CgiHandler &other);

    CgiInfo startCgi(const Request& req, const std::string& scriptPath,
                     const std::string& interpreter = "");

private:
    std::string _interpreterFor(const std::string& scriptPath) const;
    bool _pathFromDir(const std::string& dir, const std::string& path, std::string& out) const;
    std::vector<std::string> _buildEnvp(const Request& req, const std::string& scriptPath);
};

#endif