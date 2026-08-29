#ifndef CGI_HANDLER_HPP
# define CGI_HANDLER_HPP

# include <string>
# include <vector>

class Request;
class Response;

class CgiHandler
{
    private:
        std::vector<std::string> _buildEnvp(const Request& req, const std::string& scriptPath);
        std::string              _readPipeOutput(int fd);
        
    public:
        CgiHandler();
        ~CgiHandler();
        CgiHandler(const CgiHandler &other);
        CgiHandler &operator=(const CgiHandler &other);
        std::string execute(const Request& req, const std::string& scriptPath);

};

#endif