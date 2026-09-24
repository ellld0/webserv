#include "../../includes/network/ServerManager.hpp"
#include "../../includes/http/Request.hpp"
#include "../../includes/http/Response.hpp"
#include <netdb.h>
#include <sstream>
#include <cctype>
#include <cstdlib>

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
	for (size_t i = 0; i < _pollFds.size(); i++) {
		if (_pollFds[i].fd >= 0) {
			close(_pollFds[i].fd);
		}
	}
	_pollFds.clear();
	_clients.clear();
	_serverSockets.clear();
}

void ServerManager::init(const std::vector<ServerConfig>& configs) {
    std::cout << "[INFO] Starting server sockets..." << std::endl;
    
    for (size_t i = 0; i < configs.size(); ++i) {
        try {
            setupSocket(configs[i]);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to open socket. " << configs[i].getPort() 
                      << ": " << e.what() << std::endl;
            throw std::runtime_error("Server initialization aborted due to bind failure.");
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
		// Runaway scripts have to be reaped on every turn, not only when the
		// server happens to be idle.
		checkCgiTimeouts();

		if (0 == poll_count) {
			// Idle turn: drop clients that stayed silent for more than 30s.
			time_t now = std::time(NULL);
			for (size_t i = 0; i < _pollFds.size(); i++) {
				int fd = _pollFds[i].fd;
				if (_serverSockets.count(fd) > 0)
					continue;
				if (_clients.count(fd) > 0) {
					if (cgiFdForClient(fd) >= 0)
						continue; // waiting on its own CGI, not idle
					if (_clients[fd].hasPendingResponse())
						continue; // answer already queued, it is not idle either
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
				const short revents = _pollFds[i].revents;
				const int active_fd = _pollFds[i].fd;

				if (revents == 0)
					continue;

				// CGI pipes are checked first: once the script exited and its
				// output was drained, the read end reports POLLHUP *without*
				// POLLIN, and that is the only notice we get that it is over.
				if (_cgiToClientMap.count(active_fd) > 0) {
					handleCgiRead(i, revents);
					continue;
				}
				if (_cgiWriteToClientMap.count(active_fd) > 0) {
					handleCgiWrite(i, revents);
					continue;
				}

				if (revents & (POLLIN | POLLHUP | POLLERR)) {
					if (_serverSockets.count(active_fd) > 0) {
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
					}
					else {
						char buffer[65536];
						int bytes_read = recv(active_fd, buffer, sizeof(buffer), 0);
						if (bytes_read > 0) {
							_clients[active_fd].appendRequest(buffer, bytes_read);
							_clients[active_fd].updateActivity();
							const std::string& raw_request = _clients[active_fd].getRequests();
							int parent_server_fd = _clients[active_fd].getServerFd();
							size_t limit = ServerConfig::parseBodySize(_serverSockets[parent_server_fd].getClientMaxBodySize());
							if (raw_request.length() > limit) {
								std::cout << "[NETWORK] Payload too big detected at Client FD: " << active_fd << std::endl;
								std::string error_response = 
									"HTTP/1.1 413 Payload Too Large\r\n"
									"Content-Type: text/html\r\n"
									"Connection: close\r\n"
									"Content-Length: 55\r\n"
									"\r\n"
									"<html><body><h1>413 Payload Too Large</h1></body></html>";
								send(active_fd, error_response.c_str(), error_response.length(), 0);
								closeFd(active_fd, i);
								continue;
							}
							// Running the full parse on every 64KB chunk of a 100MB
							// body is quadratic: only parse once it is all here.
							if (!requestLooksComplete(raw_request))
								continue;
							Request client_request;
                            bool result = client_request.parse(raw_request);
                            if (result == false)
                                continue;
							_clients[active_fd].clearRequest();
							ServerConfig current_config = _serverSockets[parent_server_fd];
                            Response client_response;
                            client_response.build(client_request, current_config);
							if (client_response.isCgi()) {
								CgiInfo cgi_state = client_response.getCgiState();
								std::cout << "[HTTP] " << client_request.getMethod() << " " << client_request.getPath()
									<< " -> CGI (pid " << cgi_state.pid << ")" << std::endl;
								_clients[active_fd].setCgiState(cgi_state);
								_clients[active_fd].setResponseObj(client_response);
								// Only needed when the body is fed through a stdin pipe
								if (cgi_state.writeFd >= 0)
									_clients[active_fd].setCgiBody(client_request.getBody());

								struct pollfd cgi_pollfd;
								cgi_pollfd.fd = cgi_state.readFd;
								cgi_pollfd.events = POLLIN;
								cgi_pollfd.revents = 0;
								_pollFds.push_back(cgi_pollfd);
								_cgiToClientMap[cgi_state.readFd] = active_fd;

								// The body is fed to the script through poll() too,
								// so a script that never reads stdin cannot block us.
								if (cgi_state.writeFd >= 0) {
									struct pollfd cgi_in_pollfd;
									cgi_in_pollfd.fd = cgi_state.writeFd;
									cgi_in_pollfd.events = POLLOUT;
									cgi_in_pollfd.revents = 0;
									_pollFds.push_back(cgi_in_pollfd);
									_cgiWriteToClientMap[cgi_state.writeFd] = active_fd;
								}
								_pollFds[i].events = 0;
							}
							else {
								std::cout << "[HTTP] " << client_request.getMethod() << " " << client_request.getPath()
									<< " -> " << client_response.getStatusCode() << std::endl;
								_clients[active_fd].appendResponse(client_response.toString());
								_pollFds[i].events = POLLOUT;
							}
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
				else if (revents & POLLOUT) {
					Client& client = _clients[active_fd];
					int bytes_sent = send(active_fd, client.pendingResponse(), client.pendingResponseSize(), 0);
					if (bytes_sent > 0) {
						client.trimResponse(bytes_sent);
						if (!client.hasPendingResponse())
							closeFd(active_fd, i);
					}
					else if (bytes_sent <= 0) {
						std::cout << "[ERROR] send() failed on FD: " << active_fd << std::endl;
						closeFd(active_fd, i);
					}
				}
			}
		}
    }
}

// Closes a client connection. Only used for client sockets: CGI pipes are
// dropped by finishCgi()/killCgi(), which also clean their own maps.
void ServerManager::closeFd(int active_fd, size_t &i) {
	// A client may go away while its script is still running: kill the script
	// instead of leaving an orphan pipe polling forever on a dead client.
	if (cgiFdForClient(active_fd) >= 0)
		killCgi(active_fd, i);

	close(active_fd);
	_clients.erase(active_fd);
	removePollFd(active_fd, i);
}

// Removes fd from the poll set, keeping the caller's loop index consistent.
void ServerManager::removePollFd(int fd, size_t &i) {
	for (size_t j = 0; j < _pollFds.size(); ++j) {
		if (_pollFds[j].fd == fd) {
			_pollFds.erase(_pollFds.begin() + j);
			if (j <= i)
				i--;
			return;
		}
	}
}

void ServerManager::setClientWritable(int client_fd) {
	for (size_t j = 0; j < _pollFds.size(); ++j) {
		if (_pollFds[j].fd == client_fd) {
			_pollFds[j].events = POLLOUT;
			return;
		}
	}
}

int ServerManager::cgiFdForClient(int client_fd) const {
	for (std::map<int, int>::const_iterator it = _cgiToClientMap.begin(); it != _cgiToClientMap.end(); ++it) {
		if (it->second == client_fd)
			return it->first;
	}
	return -1;
}

// Drains the script output. On a non-blocking pipe read() < 0 only means
// "nothing right now", so EOF is what read() == 0 reports - or, when the buffer
// is already empty, the POLLHUP that poll() raises on a hung up pipe.
void ServerManager::handleCgiRead(size_t &i, short revents) {
    const int   cgi_fd = _pollFds[i].fd;
    const int   client_fd = _cgiToClientMap[cgi_fd];
    char        buffer[65536];
    bool        eof = false;

    // Defensive only: closeFd() -> killCgi() already kills and reaps the script
    // and drops this pipe when its client goes away, so this should not happen.
    if (_clients.count(client_fd) == 0) {
        close(cgi_fd);
        _cgiToClientMap.erase(cgi_fd);
        removePollFd(cgi_fd, i);
        return;
    }

    Response& response = _clients[client_fd].getResponseObj();
    while (true) {
        int bytes_read = read(cgi_fd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            response.appendCgiOutput(buffer, bytes_read);
            continue;
        }
        if (bytes_read == 0)
            eof = true;
        break; // < 0: nothing more for now, poll() will call us again
    }

    if (!eof && (revents & (POLLHUP | POLLERR | POLLNVAL)))
        eof = true;
    // finishCgi() is the only place that answers the client and drops the pipe,
    // so the fd never stays in _cgiToClientMap after it was closed and reused.
    if (eof)
        finishCgi(cgi_fd, client_fd, i);
}

// Feeds the request body to the script's stdin, one poll() turn at a time.
void ServerManager::handleCgiWrite(size_t &i, short revents) {
	const int	write_fd = _pollFds[i].fd;
	const int	client_fd = _cgiWriteToClientMap[write_fd];

	if (_clients.count(client_fd) == 0 || (revents & (POLLERR | POLLHUP | POLLNVAL))) {
		close(write_fd);
		_cgiWriteToClientMap.erase(write_fd);
		removePollFd(write_fd, i);
		return;
	}

	const std::string& body = _clients[client_fd].getCgiBody();
	int bytes_written = write(write_fd, body.c_str(), body.size());

	if (bytes_written > 0)
		_clients[client_fd].trimCgiBody(bytes_written);
	if (bytes_written < 0 || _clients[client_fd].getCgiBody().empty()) {
		// Closing is what gives the script EOF on stdin.
		close(write_fd);
		_cgiWriteToClientMap.erase(write_fd);
		removePollFd(write_fd, i);
	}
}

// The script is done: reap it, turn its output into the response and hand the
// client back to the poll loop as writable.
void ServerManager::finishCgi(int cgi_fd, int client_fd, size_t &i) {
	int status = 0;
	const pid_t pid = _clients[client_fd].getCgiState().pid;

	waitpid(pid, &status, 0);

	Response& response = _clients[client_fd].getResponseObj();
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		std::cout << "[CGI] pid " << pid << " failed (Client FD " << client_fd << ")" << std::endl;
		response.buildCgiError(_serverSockets[_clients[client_fd].getServerFd()], 502);
	}
	else
		response.finalizeCgi();
	std::cout << "[CGI] pid " << pid << " -> " << response.getStatusCode() << std::endl;

	_clients[client_fd].appendResponse(response.toString());
	response.releaseBody();
	setClientWritable(client_fd);

	close(cgi_fd);
	_cgiToClientMap.erase(cgi_fd);
	removePollFd(cgi_fd, i);

	// The stdin pipe may still be open if the script exited without reading it.
	const int write_fd = _clients[client_fd].getCgiState().writeFd;
	if (write_fd >= 0 && _cgiWriteToClientMap.count(write_fd) > 0) {
		close(write_fd);
		_cgiWriteToClientMap.erase(write_fd);
		removePollFd(write_fd, i);
	}
}

// Kills a running script and drops both of its pipes, without answering the
// client (used when the client itself is going away).
void ServerManager::killCgi(int client_fd, size_t &i) {
	const int	cgi_fd = cgiFdForClient(client_fd);
	const pid_t	pid = _clients[client_fd].getCgiState().pid;
	const int	write_fd = _clients[client_fd].getCgiState().writeFd;

	if (pid > 0) {
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
	}
	if (cgi_fd >= 0) {
		close(cgi_fd);
		_cgiToClientMap.erase(cgi_fd);
		removePollFd(cgi_fd, i);
	}
	if (write_fd >= 0 && _cgiWriteToClientMap.count(write_fd) > 0) {
		close(write_fd);
		_cgiWriteToClientMap.erase(write_fd);
		removePollFd(write_fd, i);
	}
}

// Scripts that never finish (www/cgi-bin/infinity_loop.py) are killed and the
// client gets a 504 instead of waiting forever.
void ServerManager::checkCgiTimeouts() {
	const time_t		now = std::time(NULL);
	std::vector<int>	expired;

	for (std::map<int, int>::iterator it = _cgiToClientMap.begin(); it != _cgiToClientMap.end(); ++it) {
		const int client_fd = it->second;
		if (_clients.count(client_fd) == 0)
			continue;
		if (now - _clients[client_fd].getCgiState().start > CGI_TIMEOUT)
			expired.push_back(client_fd);
	}

	for (size_t k = 0; k < expired.size(); ++k) {
		const int	client_fd = expired[k];
		size_t		dummy = _pollFds.size();

		std::cout << "[CGI] Timeout, killing script for Client FD: " << client_fd << std::endl;
		killCgi(client_fd, dummy);

		Response& response = _clients[client_fd].getResponseObj();
		response.buildCgiError(_serverSockets[_clients[client_fd].getServerFd()], 504);
		_clients[client_fd].appendResponse(response.toString());
		setClientWritable(client_fd);
	}
}


// Cheap check, run on every recv(): only looks at the (small) header section
// and at the tail of the buffer, never scans the whole body.
bool ServerManager::requestLooksComplete(const std::string& raw) {
	const size_t header_end = raw.find("\r\n\r\n");
	if (header_end == std::string::npos)
		return false;

	std::string headers = raw.substr(0, header_end);
	for (size_t k = 0; k < headers.size(); ++k)
		headers[k] = std::tolower(headers[k]);
	const size_t body_len = raw.size() - (header_end + 4);

	const size_t te = headers.find("\ntransfer-encoding:");
	if (te != std::string::npos && headers.find("chunked", te) != std::string::npos
		&& headers.find("chunked", te) < headers.find('\n', te + 1)) {
		static const std::string last_chunk = "0\r\n\r\n";
		if (body_len == last_chunk.size())
			return raw.compare(raw.size() - last_chunk.size(), last_chunk.size(), last_chunk) == 0;
		// Ends with "<CRLF closing the previous chunk>0\r\n\r\n"
		static const std::string terminator = "\r\n0\r\n\r\n";
		return body_len > terminator.size()
			&& raw.compare(raw.size() - terminator.size(), terminator.size(), terminator) == 0;
	}

	const size_t cl = headers.find("\ncontent-length:");
	if (cl != std::string::npos) {
		const size_t expected = std::strtoul(headers.c_str() + cl + 16, NULL, 10);
		return body_len >= expected;
	}
	return true;
}
