#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <vector>
#include "ServerConfig.hpp"

class ServerManager {
private:
    std::vector<ServerConfig> _configs;
    std::vector<int> _serverSockets;
	std::map<int, Client> _clients;

    void setupSockets(); 

public:
    ServerManager();
    ServerManager(const ServerManager& other);
    ServerManager& operator=(const ServerManager& other);
    ~ServerManager();

    void init(const std::vector<ServerConfig>& configs); // Função responsável por inicializar o Servidor

    void run(); // O loop infinito do poll() (ou epoll/kqueue) - Nunca deve bloquear!
};

#endif
