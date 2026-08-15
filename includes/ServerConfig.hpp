#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include "LocationConfig.hpp"
#include <map>
#include <functional>

class ServerConfig {
	private:
		int port_;
		std::string serverName_;
		std::string clientMaxBodySize_;
		std::vector<LocationConfig> locations_;
		std::map<int, std::string> errorPages_;

	public:
		ServerConfig();
		ServerConfig(const ServerConfig& other);
		ServerConfig& operator=(const ServerConfig& other);
		~ServerConfig();
		void setPort(int port);
		int getPort() const;
		void setServerName(const std::string& serverName);
		const std::string& getServerName() const;
		void setClientMaxBodySize(const std::string& size);
		const std::string& getClientMaxBodySize() const;
		void addLocation(const LocationConfig& location);
		const std::vector<LocationConfig>& getLocations() const;
		void setErrorPage(int errorCode, const std::string& page);
		const std::map<int, std::string>& getErrorPages() const;
		std::string getErrorPage(int errorCode) const;

};

#endif // SERVERCONFIG_HPP
