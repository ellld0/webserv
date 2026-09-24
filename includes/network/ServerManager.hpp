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
#include <sys/wait.h>


#define CGI_TIMEOUT 30

class ServerManager {
	private:
		std::vector<struct pollfd>	_pollFds;
		std::map<int, Client> 		_clients;
		std::map<int, ServerConfig>	_serverSockets;
		std::map<int, int>			_cgiToClientMap;
		std::map<int, int>			_cgiWriteToClientMap;

		void	setupSocket(const ServerConfig& config);
		void	closeFd(int active_fd, size_t &i);
		size_t	parseBodySize(const std::string& size_str);
		static bool	requestLooksComplete(const std::string& raw);

		void	handleCgiRead(size_t &i, short revents);
		void	handleCgiWrite(size_t &i, short revents);
		void	finishCgi(int cgi_fd, int client_fd, size_t &i);
		void	killCgi(int client_fd, size_t &i);
		void	removePollFd(int fd, size_t &i);
		void	setClientWritable(int client_fd);
		void	checkCgiTimeouts();
		int		cgiFdForClient(int client_fd) const;
	public:
		ServerManager();
		ServerManager(const ServerManager& other);
		ServerManager& operator=(const ServerManager& other);
		~ServerManager();

		void init(const std::vector<ServerConfig>& configs);
		void run();
};

#endif
