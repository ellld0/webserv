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


bool Request::parse(const std::string& rawBuffer)
{
    std::string separator = "\r\n\r\n";
    size_t headerEndPos = rawBuffer.find(separator);

    if (headerEndPos == std::string::npos) {
        _complete = false;
        return false;
    }

    size_t current_body_length = rawBuffer.length() - (headerEndPos + separator.size());

    std::string headerSection = rawBuffer.substr(0, headerEndPos);

    std::istringstream ss(headerSection);
    std::string requestLine;
    std::getline(ss, requestLine);

    if (!requestLine.empty() && requestLine[requestLine.size() - 1] == '\r')
        requestLine.erase(requestLine.size() - 1);

    _parseRequestLine(requestLine);

    std::string headersRaw = headerSection.substr(requestLine.size() + 2);
    
    _parseHeaders(headersRaw);

    std::string tEncStr = getHeader("Transfer-Encoding");
    if (tEncStr.empty()) tEncStr = getHeader("transfer-encoding");

    if (_method == "POST") {
        std::string cLenStr = getHeader("Content-Length");
        if (cLenStr.empty()) cLenStr = getHeader("Content-length");
        if (cLenStr.empty()) cLenStr = getHeader("content-length");
        
        if (!cLenStr.empty()) {
            size_t expected_length = std::strtoul(cLenStr.c_str(), NULL, 10);
            
            if (current_body_length < expected_length) {
                _complete = false;
                return false;
            }
        }
        else if (tEncStr.find("chunked") != std::string::npos) {
            if (rawBuffer.find("0\r\n\r\n") == std::string::npos) {
                _complete = false;
                return false;
            }
        }
    }

    if (tEncStr.find("chunked") != std::string::npos) {
        size_t pos = headerEndPos + separator.size();
            _body.reserve(rawBuffer.length());
        
        while (pos < rawBuffer.length()) {
            size_t lineEnd = rawBuffer.find("\r\n", pos);
            if (lineEnd == std::string::npos) break;
            
            std::string hexStr = rawBuffer.substr(pos, lineEnd - pos);
            size_t chunkSize = std::strtoul(hexStr.c_str(), NULL, 16);
            
            if (chunkSize == 0) break;
            
            pos = lineEnd + 2;
            _body.append(rawBuffer, pos, chunkSize);
            pos += chunkSize + 2;
        }
    } else {
        _body = rawBuffer.substr(headerEndPos + separator.size());
    }
    
    _complete = true;
    return true;
}

bool Request::isComplete() const { return _complete; }

std::string Request::getMethod() const { return _method; }

std::string Request::getPath() const { return _path; }

std::string Request::getQueryString() const { return _queryString; }

std::string Request::getHttpVersion() const { return _httpVersion; }

const std::string& Request::getBody() const { return _body; }

std::string Request::getHeader(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    
    if (it == _headers.end())
        return "";

    return it->second;
}