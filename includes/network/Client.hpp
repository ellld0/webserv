#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <ctime>
#include <string>

class Client {
	public:
		Client();
		Client(int serverFd, int clientFd);
		Client(const Client& other);
		Client& operator=(const Client& other);
		~Client();

		int			getServerFd() const;
		int			getClientFd() const;
		std::string getResponses() const;
		std::string getRequests() const;
		time_t		getLastActivity() const;

		void appendResponse(std::string response);
		void appendRequest(std::string request);
		void trimResponse(int bytes_sent);
		void updateActivity();

	private:
		int 		_serverFd;
		int			_clientFd;
		std::string	_responses;
		std::string	_requests;
		time_t		_lastActivity;
};

#endif
