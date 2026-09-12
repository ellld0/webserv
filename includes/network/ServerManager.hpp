#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <vector>
#include <map>
#include <iostream>
#include <stdexcept>
#include "../config/ServerConfig.hpp"
#include "./Client.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <ctime>
#include <cstdlib>


class ServerManager {
	private:
		void setupSocket(const ServerConfig& config);
		std::vector<struct pollfd> _pollFds;
		std::map<int, Client> _clients;
		std::map<int, ServerConfig> _serverSockets;
		void closeFd(int active_fd, size_t &i);
		size_t parseBodySize(const std::string& size_str);
	public:
		ServerManager();
		ServerManager(const ServerManager& other);
		ServerManager& operator=(const ServerManager& other);
		~ServerManager();
		void init(const std::vector<ServerConfig>& configs);
		void run();
};

#endif
