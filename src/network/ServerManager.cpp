#include "../../includes/network/ServerManager.hpp"
#include "../../includes/http/Request.hpp"
#include "../../includes/http/Response.hpp"

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
			std::cout << "[DEBUG] Idle Server, checking for dead clients..." << std::endl;
			// Future clean dead client function
			time_t now = std::time(NULL);
			for (size_t i = 0; i < _pollFds.size(); i++) {
				int fd = _pollFds[i].fd;
				if (_serverSockets.count(fd) > 0)
					continue;
				if (_clients.count(fd) > 0) {
					if (now - _clients[fd].getLastActivity() > 30) { 
						std::cout << "[NETWORK] Client from FD " << fd << " is inactive." << std::endl;
						closeFd(fd, i);
					}
				}
			}
			continue; 
		}
		else {
			for (size_t i = 0; i < _pollFds.size(); i++) {
				if (_pollFds[i].revents & POLLIN) {
					int active_fd = _pollFds[i].fd;
					if (_serverSockets.count(active_fd) > 0) {
						std::cout << "[NETWORK] New client! AT SERVER FD: " << active_fd << ")" << std::endl;
						struct sockaddr_in client_addr;
						std::memset(&client_addr, 0, sizeof(client_addr));
						socklen_t client_len = sizeof(client_addr) ;
						int client_fd = accept(active_fd, (struct sockaddr*)&client_addr, &client_len);
						if (client_fd < 0) {
							std::cout << "[ERROR] Failed to open client socket(FD) - connection not accepted." << std::endl;
							continue;
						}
						_clients[client_fd] = Client(active_fd, client_fd);
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
							std::cout << "[NETWORK] Request received from Client at FD: " << active_fd << std::endl;
							std::cout << buffer << std::endl;
							std::cout << "-----------------------------" << std::endl;
							_clients[active_fd].appendRequest(buffer);
							_clients[active_fd].updateActivity();
							int parent_server_fd = _clients[active_fd].getServerFd();
							size_t limit = parseBodySize(_serverSockets[parent_server_fd].getClientMaxBodySize());
							if (_clients[active_fd].getRequests().length() > limit) {
								std::cout << "[NETWORK] Payload too big detected at Client FD: " << active_fd << std::endl;
								closeFd(active_fd, i);
								continue;
							}
							Request client_request;
                            bool result = client_request.parse(_clients[active_fd].getRequests());
                            if (result == false)
                                continue;
							std::cout << "[NETWORK] Request 100% received from Client at FD: " << active_fd << std::endl;
							ServerConfig current_config = _serverSockets[parent_server_fd];
                            Response client_response;
                            client_response.build(client_request, current_config);
							_clients[active_fd].appendResponse(client_response.toString());
							_pollFds[i].events = POLLOUT;
						}
						else if (bytes_read == 0) {
							std::cout << "[NETWORK] Client disconnected from FD: " << active_fd << std::endl;
							closeFd(active_fd, i);
						}
						else {
							std::cout << "[ERROR] recv() failed on FD: " << active_fd << std::endl;
							closeFd(active_fd, i);
						}
					}
				}
				else if (_pollFds[i].revents & POLLOUT) {
					int	active_fd = _pollFds[i].fd;
					std::string response_str = _clients[active_fd].getResponses();
					int bytes_sent = send(active_fd, response_str.c_str(), response_str.length(), 0);
					if (bytes_sent > 0) {
						_clients[active_fd].trimResponse(bytes_sent);
						if (_clients[active_fd].getResponses().empty()) {
							std::cout << "[NETWORK] Response 100% sent. Closing FD: " << active_fd << std::endl;
							closeFd(active_fd, i);
						}
					}
					else if (bytes_sent < 0) {
						std::cout << "[NETWORK] Failed to sent response to FD: " << active_fd << std::endl;
						closeFd(active_fd, i);
					}
				}
			}
		}
    }
}

void ServerManager::closeFd(int active_fd, size_t &i) {
	std::cout << "[NETWORK] Closing connection on FD: " << active_fd << std::endl;
	close(active_fd);
	_clients.erase(active_fd);
	_pollFds.erase(_pollFds.begin() + i);
	i--;
}


size_t ServerManager::parseBodySize(const std::string& size_str) {
    if (size_str.empty()) {
        return 1048576; // Padrão seguro de 1MB se a string vier vazia
    }

    char* end;
    // O strtoul lê os números e para na primeira letra. O 'end' aponta para essa letra.
    size_t size = std::strtoul(size_str.c_str(), &end, 10);

    // Verificamos qual letra sobrou no ponteiro end
    if (*end == 'M' || *end == 'm') {
        size *= (1024 * 1024); // Transforma Megabytes em Bytes
    } 
    else if (*end == 'K' || *end == 'k') {
        size *= 1024;          // Transforma Kilobytes em Bytes
    }
    else if (*end == 'G' || *end == 'g') {
        size *= (1024 * 1024 * 1024); // Transforma Gigabytes em Bytes
    }
    
    // Se não tiver letra nenhuma, ele já leu os bytes diretos e retorna o 'size' original
    return size;
}