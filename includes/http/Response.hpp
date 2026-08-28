#ifndef RESPONSE_HPP
# define RESPONSE_HPP

class Request;

class Response
{
    private:
        int                                 _statusCode;
        std::string                         _statusMessage;
        std::map<std::string, std::string>  _headers;
        std::string                         _body;

    public:
        Response();
        ~Response();

        void build(const Request& req, const ServerConfig& config);
        std::string toString() const;
        int getStatusCode() const;
};

#endif


// Atributos privados: statusCode, statusMessage, headers (map), body
// Métodos públicos:
//   void build(const Request& req, const ServerConfig& config)
//   std::string toString() const
//   int getStatusCode() const