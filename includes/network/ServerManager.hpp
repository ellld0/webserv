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

class ServerManager {
private:
    std::vector<struct pollfd> _pollFds;
    std::map<int, ServerConfig> _serverSockets;
	std::map<int, int> _clientToServer;
	void setupSocket(const ServerConfig& config);
public:
    ServerManager();
    ServerManager(const ServerManager& other);
    ServerManager& operator=(const ServerManager& other);
    ~ServerManager();

    void init(const std::vector<ServerConfig>& configs);

    void run();
};

#endif
