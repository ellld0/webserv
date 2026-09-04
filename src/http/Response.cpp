#include "../../includes/http/Response.hpp"
#include "../../includes/http/Request.hpp"
#include "../../includes/http/CgiHandler.hpp"

#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

Response::Response() : _statusCode(200), _statusMessage("OK"), _headers(), _body() {}

Response::~Response() {}

Response::Response(const Response &other)
{
    _statusCode = other._statusCode;
    _statusMessage = other._statusMessage;
    _headers = other._headers;
    _body = other._body;
}

Response &Response::operator=(const Response &other)
{
    if (this != &other)
    {
        _statusCode = other._statusCode;
        _statusMessage = other._statusMessage;
        _headers = other._headers;
        _body = other._body;
    }
    return *this;
}

void Response::_reset()
{
    _statusCode = 200;
    _statusMessage = "OK";
    _headers.clear();
    _body.clear();
}

void Response::_setStatus(int code)
{
    _statusCode = code;
    _statusMessage = _reasonPhrase(code);
}

void Response::_setHeader(const std::string& key, const std::string& value)
{
    _headers[key] = value;
}

std::string Response::_reasonPhrase(int code) const
{
    switch (code)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        default:  return "Unknown";
    }
}

bool Response::_fileExists(const std::string& path) const
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && ((st.st_mode & S_IFMT) == S_IFREG));
}

bool Response::_loadFile(const std::string& path, std::string& out) const
{
    std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary);
    if (!ifs.is_open())
        return false;

    std::ostringstream ss;
    ss << ifs.rdbuf();
    out = ss.str();
    return true;
}

bool Response::_writeFile(const std::string& path, const std::string& content) const
{
    std::ofstream ofs(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!ofs.is_open())
        return false;

    ofs.write(content.c_str(), static_cast<std::streamsize>(content.size()));
    return ofs.good();
}

std::string Response::_contentTypeFor(const std::string& path) const
{
    std::string::size_type dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "application/octet-stream";

    const std::string ext = path.substr(dot + 1);
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "txt") return "text/plain";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "svg") return "image/svg+xml";
    return "application/octet-stream";
}

std::string Response::_httpDateNow() const
{
    std::time_t now = std::time(NULL);
    std::tm* gmt = std::gmtime(&now);
    char buf[128];

    if (!gmt)
        return "";

    if (std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt) == 0)
        return "";

    return std::string(buf);
}

bool Response::_isPathSafe(const std::string& path) const
{
    if (path.empty() || path[0] != '/')
        return false;
    if (path.find("..") != std::string::npos)
        return false;
    return true;
}

bool Response::_isCgiTarget(const std::string& path) const
{
    std::string::size_type dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return false;

    const std::string ext = path.substr(dot + 1);
    return (ext == "py" || ext == "php" || ext == "cgi" || ext == "sh");
}

std::string Response::_resolveTargetPath(const Request& req) const
{
    std::string target = req.getPath();
    if (target == "/" || target.empty())
        target = "/index.html";
    return target;
}

LocationConfig* Response::_resolveLocation(const Request& req, const ServerConfig& config) const
{
    const std::string& path = req.getPath();
    const std::vector<LocationConfig>& locations = config.getLocations();

    for (size_t i = 0; i < locations.size(); ++i)
    {
        if (locations[i].getPath() == path)
            return const_cast<LocationConfig*>(&locations[i]);
    }

    for (size_t i = 0; i < locations.size(); ++i)
    {
        if (locations[i].getPath() == "/")
            return const_cast<LocationConfig*>(&locations[i]);
    }

    return NULL;
}

bool Response::_isMethodAllowed(const Request& req, const LocationConfig& location) const
{
    const std::string& method = req.getMethod();
    const std::vector<std::string>& allowed = location.getMethods();

    for (size_t i = 0; i < allowed.size(); ++i)
    {
        if (allowed[i] == method)
            return true;
    }

    return false;
}


void Response::_serveStaticFile(const std::string& fullPath)
{
    if (!_fileExists(fullPath))
    {
        _setStatus(404);
        return;
    }

    if (!_loadFile(fullPath, _body))
    {
        _setStatus(500);
        return;
    }

    _setStatus(200);
    _setHeader("Content-Type", _contentTypeFor(fullPath));
}

void Response::_parseCgiOutput(const std::string& rawOutput)
{
    std::string::size_type sep = rawOutput.find("\r\n\r\n");
    std::string separator = "\r\n\r\n";

    if (sep == std::string::npos)
    {
        sep = rawOutput.find("\n\n");
        separator = "\n\n";
    }

    if (sep == std::string::npos)
    {
        _body = rawOutput;
        if (_headers.find("Content-Type") == _headers.end())
            _setHeader("Content-Type", "text/html");
        return;
    }

    const std::string rawHeaders = rawOutput.substr(0, sep);
    _body = rawOutput.substr(sep + separator.size());

    std::istringstream hs(rawHeaders);
    std::string line;
    while (std::getline(hs, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        std::string::size_type colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string key = line.substr(0, colon);
        std::string value;
        if (colon + 1 < line.size())
            value = line.substr(colon + 1);

        if (!value.empty() && value[0] == ' ')
            value.erase(0, 1);

        if (key == "Status")
        {
            std::istringstream ss(value);
            int code = 0;
            ss >> code;
            if (code > 0)
                _setStatus(code);
        }
        else
        {
            _setHeader(key, value);
        }
    }

    if (_headers.find("Content-Type") == _headers.end())
        _setHeader("Content-Type", "text/html");
}

void Response::_handleCgi(const Request& req, const std::string& scriptPath)
{
    CgiHandler cgi;
    try
    {
        const std::string raw = cgi.execute(req, scriptPath);
        _setStatus(200);
        _parseCgiOutput(raw);
    }
    catch (...)
    {
        _setStatus(500);
    }
}
void Response::_finalizeHeaders()
{
    std::ostringstream lengthStream;
    lengthStream << _body.size();
    _setHeader("Content-Length", lengthStream.str());
    _setHeader("Date", _httpDateNow());
    _setHeader("Connection", "close");
    _setHeader("Server", "WebServer");
}

void Response::_handleGet(const Request& req, const ServerConfig& config)
{
    LocationConfig* location = _resolveLocation(req, config);

    if(!location)
    {
        _setStatus(404);
        return;
    }
    if(!_isMethodAllowed(req, *location))
    {
        _setStatus(405);
        return;
    }
    if(location->getRedirectCode() > 0)
    {
        _setStatus(location->getRedirectCode());
        _setHeader("Location", location->getRedirectUrl());
        _body.clear();
        return;
    }
    std::string root = location->getRoot();
    std::string targetPath = _resolveTargetPath(req);
    std::string fullPath = root + targetPath;

    if(_isCgiTarget(fullPath))
    {
        _handleCgi(req, fullPath);
        return;
    }
    
    _serveStaticFile(fullPath);
}

void Response::_handlePost(const Request& req, const ServerConfig& config)
{
    LocationConfig* location = _resolveLocation(req, config);

    if(!location)
    {
        _setStatus(404);
        return;
    }
    if(!_isMethodAllowed(req, *location))
    {
        _setStatus(405);
        return;
    }
    std::string maxBodySize = config.getClientMaxBodySize();
    size_t maxBytes = _parseBodySize(maxBodySize);

    if(req.getBody().size() > maxBytes)
    {
        _setStatus(413);
        return;
    }

    std::string root = location->getRoot();
    std::string targetPath = _resolveTargetPath(req);
    std::string fullPath = root + targetPath;

    if(_isCgiTarget(fullPath))
    {
        _handleCgi(req, fullPath);
        return;
    }

    std::string uploadPath = location->getUploadPath();
    if(!uploadPath.empty())
    {
        std::string fileName = uploadPath + "/upload.bin";
        if(!_writeFile(fileName, req.getBody()))
        {
            _setStatus(500);
            return;
        }

        _setStatus(201);
        _body = "File uploaded successfully.\n";
        _setHeader("Content-Type", "text/plain");
    }
    else
    {
        _setStatus(405);
    }
}
void Response::_handleDelete(const Request& req, const ServerConfig& config)
{
    LocationConfig* location = _resolveLocation(req, config);

    if(!location)
    {
        _setStatus(404);
        return;
    }
    if (!_isMethodAllowed(req, *location))
    {
        _setStatus(405);
        return;
    }

    std::string root = location->getRoot();
    std::string targetPath = _resolveTargetPath(req);
    std::string fullPath = root + targetPath;

    if (!_fileExists(fullPath))
    {
        _setStatus(404);
        return;
    }
    if (std::remove(fullPath.c_str()) != 0)
    {
        _setStatus(403);
        return;
    }
    _setStatus(200);
    _body.clear();
}

void Response::_buildErrorResponse(const ServerConfig& config, int code)
{
    _setStatus(code);

    std::string customPath = config.getErrorPage(code);
    if (!customPath.empty() && _loadFile(customPath, _body))
    {
        _setHeader("Content-Type", _contentTypeFor(customPath));
        return;
    }

    if (code == 404 && _loadFile("www/404.html", _body))
    {
        _setHeader("Content-Type", "text/html");
        return;
    }

    std::ostringstream ss;
    ss << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
       << "<title>" << _statusCode << " " << _statusMessage << "</title></head>"
       << "<body><h1>" << _statusCode << " " << _statusMessage << "</h1></body></html>";
    _body = ss.str();
    _setHeader("Content-Type", "text/html");
}

size_t Response::_parseBodySize(const std::string& sizeStr) const
{
    if (sizeStr.empty())
        return 1024UL * 1024UL; // 1MB default

    char lastChar = sizeStr[sizeStr.size() - 1];
    std::string numPart = sizeStr.substr(0, sizeStr.size() - 1);
    size_t base = static_cast<size_t>(std::atol(numPart.c_str()));

    if (lastChar == 'K')
        return base * 1024UL;
    if (lastChar == 'M')
        return base * 1024UL * 1024UL;
    if (lastChar == 'G')
        return base * 1024UL * 1024UL * 1024UL;

    return base;
}

void Response::build(const Request& req, const ServerConfig& config)
{
    _reset();

    const std::string method = req.getMethod();
    const std::string path = req.getPath();

    if (!_isPathSafe(path))
    {
        _buildErrorResponse(config, 400);
        _finalizeHeaders();
        return;
    }

    if (method == "GET")
        _handleGet(req, config);
    else if (method == "POST")
        _handlePost(req, config);
    else if (method == "DELETE")
        _handleDelete(req, config);
    else
        _setStatus(501);

    if (_statusCode >= 400)
        _buildErrorResponse(config, _statusCode);

    _finalizeHeaders();
}

std::string Response::toString() const
{
    std::ostringstream ss;
    ss << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";

    std::map<std::string, std::string>::const_iterator it = _headers.begin();
    while (it != _headers.end())
    {
        ss << it->first << ": " << it->second << "\r\n";
        ++it;
    }

    ss << "\r\n";
    ss << _body;
    return ss.str();
}

int Response::getStatusCode() const
{
    return _statusCode;
}