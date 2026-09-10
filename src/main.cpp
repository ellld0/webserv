#include <iostream>
#include <string>
#include <exception>
#include <csignal>
#include <cstring>

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
		ConfigParser parser;
		std::vector<ServerConfig> servers = parser.parseConfigFile(configFile);
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