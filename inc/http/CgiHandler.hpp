#ifndef CGI_HANDLER_HPP
# define CGI_HANDLER_HPP
# include <string>

class Request;
class Response;

class CgiHandler
{
    public:
        std::string execute(const Request& req, const std::string& scriptPath);

    private:
        std::vector<std::string> _buildEnvp(const Request& req, const std::string& scriptPath);
        std::string              _readPipeOutput(int fd);
};

#endif