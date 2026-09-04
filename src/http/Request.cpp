#include "../../includes/http/Request.hpp"

Request::Request() : _complete(false) {}
Request::~Request() {}

Request::Request(const Request &other)
{
    (void)other;
}

Request &Request::operator=(const Request &other)
{
    (void)other;
    return *this;
}

void Request::_parseRequestLine(const std::string& line)
{
    std::istringstream ss(line);
    ss >> _method;
    ss >> _path;
    ss >> _httpVersion;

    size_t queryPos = _path.find('?');

    if (queryPos != std::string::npos)
    {
        _queryString = _path.substr(queryPos + 1);
        _path = _path.substr(0, queryPos);
    }
}

void Request::_parseHeaders(const std::string& rawHeaders)
{
    std::istringstream ss(rawHeaders);
    std::string line;

    while (std::getline(ss, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (line.empty())
            continue;

        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;

        std::string headerName = line.substr(0, colonPos);
        std::string headerValue;
        if (colonPos + 1 < line.size())
            headerValue = line.substr(colonPos + 1);

        while (!headerValue.empty() && (headerValue[0] == ' ' || headerValue[0] == '\t'))
            headerValue.erase(0, 1);

        _headers[headerName] = headerValue;
    }
}

// Método publico principal para parsear a requisição HTTP

bool Request::parse(const std::string& rawBuffer)
{
    std::string separator = "\r\n\r\n";
    size_t headerEndPos = rawBuffer.find(separator);

    if (headerEndPos == std::string::npos)
    {
        _complete = false;
        return false;
    }

    std::string headerSection = rawBuffer.substr(0, headerEndPos);

    _body = rawBuffer.substr(headerEndPos + separator.size());

    std::istringstream ss(headerSection);
    std::string requestLine;
    std::getline(ss, requestLine);

    if (!requestLine.empty() && requestLine[requestLine.size() - 1] == '\r')
        requestLine.erase(requestLine.size() - 1);

    _parseRequestLine(requestLine);

    std::string headersRaw = headerSection.substr(requestLine.size() + 2);

    _parseHeaders(headersRaw);
    _complete = true;
    return true;
}

bool Request::isComplete() const { return _complete; }

std::string Request::getMethod() const { return _method; }

std::string Request::getPath() const { return _path; }

std::string Request::getQueryString() const { return _queryString; }

std::string Request::getHttpVersion() const { return _httpVersion; }

std::string Request::getBody() const { return _body; }

std::string Request::getHeader(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    
    if (it == _headers.end())
        return "";

    return it->second;
}