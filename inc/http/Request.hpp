#ifndef REQUEST_HPP
# define REQUEST_HPP
# include <string>
# include <map>


class Request
{
    private:
        std::string                         _method;
        std::string                         _path;
        std::string                         _queryString;
        std::string                         _httpVersion;
        std::map<std::string, std::string>  _headers;
        std::string                         _body;
        bool                                _complete;

        void _parseRequestLine(const std::string& requestLine);
        void _parseHeaders(const std::string& headers);

    public:
         Request();
        ~Request();

        bool        parse(const std::string& rawBuffer);
        bool        isComplete() const;

        std::string getMethod() const;
        std::string getPath() const;
        std::string getQueryString() const;
        std::string getHttpVersion() const;
        std::string getHeader(const std::string& key) const;
        std::string getBody() const;
};

#endif