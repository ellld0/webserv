#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <ctime>
#include <string>
#include "../http/CgiHandler.hpp"
#include "../http/Response.hpp"

class Client {
	public:
		Client();
		Client(int serverFd, int clientFd);
		Client(const Client& other);
		Client& operator=(const Client& other);
		~Client();

		int			getServerFd() const;
		int			getClientFd() const;
		const std::string&	getRequests() const;
		const char*	pendingResponse() const;
		size_t		pendingResponseSize() const;
		bool		hasPendingResponse() const;
		void		clearRequest();
		time_t		getLastActivity() const;
		CgiInfo		getCgiState() const { return _cgiState; }
		void 		setCgiState(const CgiInfo& cgiState) { _cgiState = cgiState; }
		void 		setResponseObj(const Response& res);
		Response& 	getResponseObj();

		const std::string&	getCgiBody() const;
		void				setCgiBody(const std::string& body);
		void				trimCgiBody(size_t bytes_sent);

		void appendResponse(const std::string& response);
		void appendRequest(const char* data, size_t len);
		void trimResponse(size_t bytes_sent);
		void updateActivity();

	private:
		int 		_serverFd;
		int			_clientFd;
		std::string	_responses;
		size_t		_responseSent;
		std::string	_requests;
		time_t		_lastActivity;
		CgiInfo		_cgiState;
		Response	_responseObj;
		std::string	_cgiBody;
};

#endif
