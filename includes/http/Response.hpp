#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <string>
# include <map>
# include "ServerConfig.hpp"

class Request;

class Response
{
    private:
        int                                 _statusCode;
        std::string                         _statusMessage;
        std::map<std::string, std::string>  _headers;
        std::string                         _body;

        void        _reset();
        void        _setStatus(int code);
        void        _setHeader(const std::string& key, const std::string& value);
        void        _finalizeHeaders();

        void        _handleGet(const Request& req, const ServerConfig& config);
        void        _handlePost(const Request& req, const ServerConfig& config);
        void        _handleDelete(const Request& req, const ServerConfig& config);

        void        _serveStaticFile(const std::string& fullPath);
        void        _handleCgi(const Request& req, const std::string& scriptPath);
        void        _parseCgiOutput(const std::string& rawOutput);

        void        _buildErrorResponse(const ServerConfig& config, int code);

        bool        _fileExists(const std::string& path) const;
        bool        _loadFile(const std::string& path, std::string& out) const;
        bool        _writeFile(const std::string& path, const std::string& content) const;

        std::string _resolveTargetPath(const Request& req) const;
        std::string _reasonPhrase(int code) const;
        std::string _contentTypeFor(const std::string& path) const;
        std::string _httpDateNow() const;
        bool        _isCgiTarget(const std::string& path) const;
        bool        _isPathSafe(const std::string& path) const;

        LocationConfig* _resolveLocation(const Request& req, const ServerConfig& config) const;
        bool        _isMethodAllowed(const Request& req, const LocationConfig& location) const;

        size_t      _parseBodySize(const std::string& sizeStr) const;

    public:
        Response();
        ~Response();
        Response(const Response &other);
        Response &operator=(const Response &other);

        void build(const Request& req, const ServerConfig& config);
        std::string toString() const;
        int getStatusCode() const;
};

#endif
