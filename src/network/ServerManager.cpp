#include "../../includes/network/ServerManager.hpp"
#include "../../includes/http/Request.hpp"
#include "../../includes/http/Response.hpp"
#include <cstring>
#include <csignal>

ServerManager::ServerManager() {}

ServerManager::ServerManager(const ServerManager& other) {
    *this = other;
}

ServerManager& ServerManager::operator=(const ServerManager& other) {
    if (this != &other) {
        this->_pollFds = other._pollFds;
        this->_serverSockets = other._serverSockets;
    }
    return *this;
}

ServerManager::~ServerManager() {
    // Need to close all FD`s open
}

void ServerManager::init(const std::vector<ServerConfig>& configs) {
    std::cout << "[INFO] Starting server sockets..." << std::endl;
    
    for (size_t i = 0; i < configs.size(); ++i) {
        try {
            setupSocket(configs[i]);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to open socket. " << configs[i].getPort() 
                      << ": " << e.what() << std::endl;
            // Need to choose what to do if one server fail. Down everything or just continue without this server?
        }
    }
}

void ServerManager::setupSocket(const ServerConfig& config) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        throw std::runtime_error("Error: Failed to create Socket!");
    }

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverFd);
        throw std::runtime_error("Erro no setsockopt (SO_REUSEADDR)");
    }

    if (fcntl(serverFd, F_SETFL, O_NONBLOCK) < 0) {
        close(serverFd);
        throw std::runtime_error("Erro to setup fcntl (O_NONBLOCK)");
    }

    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(config.getPort()); 

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(serverFd);
        throw std::runtime_error("Error: on bind. Maybe address already in use.");
    }

    if (listen(serverFd, SOMAXCONN) < 0) {
        close(serverFd);
        throw std::runtime_error("Error: Listen()");
    }

    struct pollfd pfd;
    pfd.fd = serverFd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    _pollFds.push_back(pfd);
    _serverSockets[serverFd] = config;

    std::cout << "[INFO] Socket successfully opened on port: " << config.getPort() 
              << " (FD: " << serverFd << ")" << std::endl;
}

void ServerManager::run() {
	if (_pollFds.empty()) {
		std::cout << "[ERROR] No sockets found to polling" << std::endl;
	}
	std::cout << "[INFO] Server listening..." << std::endl;
    while (true) {
		int poll_count = poll(_pollFds.data(), _pollFds.size(), 1000);
		
		if (poll_count < 0) {
			std::cout << "[ERROR] poll() failed." << std::endl;
			break;
		}
		else if (0 == poll_count) {
			//std::cout << "[DEBUG] Idle Server, checking for dead clients..." << std::endl;
			// Future clean dead client function
			continue; 
		}
		else {
			for (size_t i = 0; i < _pollFds.size(); i++) {
				if (_pollFds[i].revents & POLLIN) {
					int active_fd = _pollFds[i].fd;
					if (_serverSockets.count(active_fd) > 0) {
						std::cout << "[NETWORK] New client! (FD: " << active_fd << ")" << std::endl;
						struct sockaddr_in client_addr;
						std::memset(&client_addr, 0, sizeof(client_addr));
						socklen_t client_len = sizeof(client_addr) ;
						int client_fd = accept(active_fd, (struct sockaddr*)&client_addr, &client_len);
						_clientToServer[client_fd] = active_fd;
						if (client_fd < 0) {
							std::cout << "[ERROR] Failed to open client FD - connection not accepted." << std::endl;
							continue;
						}
						fcntl(client_fd, F_SETFL, O_NONBLOCK);
						struct pollfd client_pfd;
						client_pfd.fd = client_fd;
						client_pfd.events = POLLIN;
						client_pfd.revents = 0;
						_pollFds.push_back(client_pfd);
						std::cout << "[NETWORK] Client accepted." << std::endl;
					}
					
					else {
						std::cout << "[NETWORK] Request from an already Client! (FD: " << active_fd << ")" << std::endl;
						char buffer[4096];
						int bytes_read = recv(active_fd, buffer, sizeof(buffer) - 1, 0);
						if (bytes_read > 0) {
							buffer[bytes_read] = 0;
							std::cout << "[NETWORK] Request received from FD: " << active_fd << std::endl;
							std::cout << buffer << std::endl;
							std::cout << "-----------------------------" << std::endl;

							Request client_request;
                            bool result = client_request.parse(buffer);
                            int parent_server_fd = _clientToServer[active_fd];
							ServerConfig current_config = _serverSockets[parent_server_fd];
                            if (result == false) {
                                std::cout << "[ERROR] Bad request." << std::endl;
                                continue;
                            }
                            
                            Response client_response;
                            // Genial o teste com o [3]! Depois vamos arrumar isso de forma dinâmica.
                            client_response.build(client_request, current_config); 
                            
                            std::string response_str = client_response.toString();
                            
                            // 1. Usamos .c_str() para converter a string do C++ para C
                            // 2. Usamos .length() para dizer quantos bytes enviar
                            send(active_fd, response_str.c_str(), response_str.length(), 0);
						}
						else if (bytes_read == 0) {
							std::cout << "[NETWORK] Client disconnected from FD: " << active_fd << std::endl;
						}
						close(active_fd);
					}
				}
			}
		}
    }
}