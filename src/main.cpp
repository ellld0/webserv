#include <iostream>
#include <string>
#include <exception>

#include "../includes/config/ConfigParser.hpp"
#include "../includes/config/ServerConfig.hpp"
#include "../includes/network/ServerManager.hpp"

int main(int argc, char **argv) {
	std::string configFile;

	if (argc == 2) {
		configFile = argv[1];
	}
	else if (argc == 1) {
		configFile = "conf/default.conf";
	}
	else {
		std::cout << "Error! Correct use is ./webserv <configFileName>" << std::endl;
		return 1;
	}

	try {
		std::cout << "[INFO] Reading config file..." << std::endl;
		ConfigParser parser; 														//#Temporary inactive, waiting for finish functions - bassiro
		std::vector<ServerConfig> servers = parser.parseConfigFile(configFile);		//#Temporary inactive, waiting for finish functions - bassiro
		
		/*This block is only for mocking data to Network test
		std::vector<ServerConfig> mockServers;

		ServerConfig server1;
		server1.setPort(8080);
		server1.setServerName("Localhost");
		server1.setClientMaxBodySize("10M");
		mockServers.push_back(server1);

		ServerConfig server2;
		server2.setPort(8081);
		server2.setServerName("Localhost2");
		server2.setClientMaxBodySize("1M");
		mockServers.push_back(server2);

		std::cout << "[INFO] Mock servers created. Total Servers: " 
              << mockServers.size() << std::cout << std::endl;
		End mocking data*/

		std::cout << "[INFO] Starting Network Handler..." << std::endl;
		ServerManager manager;
		manager.init(servers);
		std::cout << "[INFO] Server is running...Press Ctrl+C to stop" << std::endl;
		manager.run();
	}
	catch (const std::exception& e) {
		std::cerr << "Fatal Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}