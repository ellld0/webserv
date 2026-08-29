#include "../../includes/network/ServerManager.hpp"
#include <cstring>
#include <csignal>

extern volatile sig_atomic_t g_server_running;

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
    while (g_server_running) {
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
						//Future accept()
					}
					else {
						std::cout << "[NETWORK] Request from an already Client! (FD: " << active_fd << ")" << std::endl;
						//Future recv()
					}
				}
			}
		}
    }
}