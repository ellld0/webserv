#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <string>
# include <map>
# include "../config/ServerConfig.hpp"
# include "../config/LocationConfig.hpp"
# include <iostream>
# include <sys/stat.h>
# include <dirent.h>
# include <unistd.h>
# include "CgiHandler.hpp"

class Request;

class Response
{
    private:
        int                                 _statusCode;
        std::string                         _statusMessage;
        std::map<std::string, std::string>  _headers;
        std::string                         _body;
		CgiInfo 							_cgiState;
    	bool 								_isCgi;
    	std::string 						_cgiRawOutput;

        void        _reset();
        void        _setStatus(int code);
        void        _setHeader(const std::string& key, const std::string& value);
        void        _finalizeHeaders();

        void        _handleGet(const Request& req, const ServerConfig& config);
        void        _handlePost(const Request& req, const ServerConfig& config);
        void        _handleDelete(const Request& req, const ServerConfig& config);

        void        _serveStaticFile(const std::string& fullPath, LocationConfig* location, const Request& req);
        void        _handleCgi(const Request& req, const std::string& scriptPath,
                               const std::string& interpreter = "");
        void        _parseCgiOutput();
		void		_generateDirectoryListing(const std::string& fullPath, const Request& req);

        void        _buildErrorResponse(const ServerConfig& config, int code);

        bool        _fileExists(const std::string& path) const;
        bool        _loadFile(const std::string& path, std::string& out) const;
        bool        _writeFile(const std::string& path, const std::string& content) const;

        std::string _resolveTargetPath(const Request& req, LocationConfig* location) const;
        std::string _reasonPhrase(int code) const;
        std::string _contentTypeFor(const std::string& path) const;
        std::string _httpDateNow() const;
        bool        _isCgiTarget(const std::string& path) const;
        bool        _isPathSafe(const std::string& path) const;

        LocationConfig* _resolveLocation(const Request& req, const ServerConfig& config) const;
        LocationConfig* _findCgiLocation(const Request& req, const ServerConfig& config) const;
        bool        _isMethodAllowed(const Request& req, const LocationConfig& location) const;


    public:
        Response();
        ~Response();
        Response(const Response &other);
        Response &operator=(const Response &other);

        void 		build(const Request& req, const ServerConfig& config);
        std::string toString() const;
        void        releaseBody();
        int 		getStatusCode() const;
		bool 		isCgi() const { return _isCgi; }
    	CgiInfo 	getCgiState() const { return _cgiState; }
		const std::string& getCgiOutput() const { return _cgiRawOutput; }
		void 		appendCgiOutput(const char* buf, size_t len);
    	void 		finalizeCgi();
    	void 		buildCgiError(const ServerConfig& config, int code);
};

#endif
