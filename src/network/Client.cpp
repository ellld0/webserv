#include "../../includes/network/Client.hpp"

Client::Client(): _serverFd(-1), _clientFd(-1), _responseSent(0), _lastActivity(std::time(NULL)) {};

Client::Client(int server_fd, int client_fd): _serverFd(server_fd), _clientFd(client_fd), _responseSent(0), _lastActivity(std::time(NULL)) {};

Client::Client(const Client& other) {
	*this = other;
}

Client& Client::operator=(const Client& other) {
	if(this != &other) {
        this->_serverFd = other._serverFd;
		this->_clientFd = other._clientFd;
        this->_responses = other._responses;
        this->_responseSent = other._responseSent;
        this->_requests = other._requests;
        this->_lastActivity = other._lastActivity;
		this->_cgiState = other._cgiState;
		this->_responseObj = other._responseObj;
		this->_cgiBody = other._cgiBody;
    }
    return *this;
}

Client::~Client() {
}

int Client::getServerFd() const {
	return this->_serverFd;
}

int Client::getClientFd() const {
	return this->_clientFd;
}

const std::string& Client::getRequests() const {
	return this->_requests;
}

const char* Client::pendingResponse() const {
	return this->_responses.data() + this->_responseSent;
}

size_t Client::pendingResponseSize() const {
	return this->_responses.size() - this->_responseSent;
}

bool Client::hasPendingResponse() const {
	return this->_responseSent < this->_responses.size();
}

// Frees the raw request once it was parsed: it can be 100MB per client.
void Client::clearRequest() {
	std::string().swap(this->_requests);
}

time_t Client::getLastActivity() const {
	return this->_lastActivity;
}

void Client::appendResponse(const std::string& response) {
	this->_responses += response;
}

void Client::appendRequest(const char* data, size_t len) {
	this->_requests.append(data, len);
}

// Only moves the offset: erasing the front of a 100MB string on every send()
// would copy the whole response again each time.
void Client::trimResponse(size_t bytes_sent) {
	this->_responseSent += bytes_sent;
	if (this->_responseSent >= this->_responses.size()) {
		std::string().swap(this->_responses);
		this->_responseSent = 0;
	}
}

void Client::updateActivity() {
	this->_lastActivity = std::time(NULL);
}

void Client::setResponseObj(const Response& res) {
    _responseObj = res;
}

Response& Client::getResponseObj() {
    return _responseObj;
}

const std::string& Client::getCgiBody() const {
	return this->_cgiBody;
}

void Client::setCgiBody(const std::string& body) {
	this->_cgiBody = body;
}

void Client::trimCgiBody(size_t bytes_sent) {
	if (bytes_sent >= this->_cgiBody.size())
		this->_cgiBody.clear();
	else
		this->_cgiBody = this->_cgiBody.substr(bytes_sent);
}
