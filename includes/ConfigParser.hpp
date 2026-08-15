#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

class ConfigParser
{
	public:
		ConfigParser();
		ConfigParser(const ConfigParser& other);
		ConfigParser& operator=(const ConfigParser& other);
		~ConfigParser();
		std::vector<ServerConfig> parseConfigFile(const std::string& filename);
	private:
		bool isValidPort(const std::string& portString);
		bool isValidBodySize(const std::string& sizeString);
		std::string preprocess(const std::string& line);
		ServerConfig parseServer(const std::vector<std::string>& tokens, size_t& i);
		LocationConfig parseLocation(const std::vector<std::string>& tokens, size_t& i);
        void handleListen(ServerConfig& server, const std::vector<std::string>& tokens, size_t& i, bool& hasListen);
        void handleServerName(ServerConfig& server, const std::vector<std::string>& tokens, size_t& i, bool& hasServerName);
        void handleErrorPage(ServerConfig& server, const std::vector<std::string>& tokens, size_t& i);
        void handleClientMaxBodySize(ServerConfig& server, const std::vector<std::string>& tokens, size_t& i, bool& hasClientMaxBodySize);
        void handleLocation(ServerConfig& server, const std::vector<std::string>& tokens, size_t& i);
		void parseRootDirective(LocationConfig& loc, const std::vector<std::string>&tokens, size_t& i, bool& hasRoot);
		void parseMethodsDirective(LocationConfig& loc, const std::vector<std::string>&tokens, size_t&, bool& hasMethod);
		void parseDirListingDirective(LocationConfig& loc, const std::vector<std::string>& tokens, size_t&, bool& hasDirListing);
		void parseReturnDirective(LocationConfig& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasReturn);
		void parseIndexDirective(LocationConfig& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasIndex);
		void parseUploadDirective(LocationConfig& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasUpload);
		

};

#endif // CONFIGPARSER_HPP
