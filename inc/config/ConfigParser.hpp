#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

class Parser
{
	public:
		Parser();
		~Parser();
		Server parseConfigFile(const std::string& filename);
	private:
		bool isValidPort(const std::string& portString);
		bool isValidBodySize(const std::string& sizeString);
		std::string preprocess(const std::string& line);
		Server parseServer(const std::vector<std::string>& tokens);
		Location parseLocation(const std::vector<std::string>& tokens, size_t& i);
        void handleListen(Server& server, const std::vector<std::string>& tokens, size_t& i, bool& hasListen);
        void handleServerName(Server& server, const std::vector<std::string>& tokens, size_t& i, bool& hasServerName);
        void handleErrorPage(Server& server, const std::vector<std::string>& tokens, size_t& i);
        void handleClientMaxBodySize(Server& server, const std::vector<std::string>& tokens, size_t& i, bool& hasClientMaxBodySize);
        void handleLocation(Server& server, const std::vector<std::string>& tokens, size_t& i);
		void parseRootDirective(Location& loc, const std::vector<std::string>&tokens, size_t& i, bool& hasRoot);
		void parseMethodsDirective(Location& loc, const std::vector<std::string>&tokens, size_t&, bool& hasMethod);
		void parseDirListingDirective(Location& loc, const std::vector<std::string>& tokens, size_t&, bool& hasDirListing);
		void parseReturnDirective(Location& loc, const std::vector<std::string>& tokens, size_t& i);
		void parseIndexDirective(Location& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasIndex);
		void parseUploadDirective(Location& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasUpload);
		

};

#endif // CONFIGPARSER_HPP
