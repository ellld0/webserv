#include "../../includes/network/Client.hpp"

Client::Client(): _serverFd(-1), _lastActivity(std::time(NULL)) {};

Client::Client(int server_fd, int client_fd): _serverFd(server_fd), _clientFd(client_fd), _lastActivity(std::time(NULL)) {};

Client::Client(const Client& other) {
	*this = other;
}

Client& Client::operator=(const Client& other) {
	if(this != &other) {
        this->_serverFd = other._serverFd;
		this->_clientFd = other._clientFd;
        this->_responses = other._responses;
        this->_requests = other._requests;
        this->_lastActivity = other._lastActivity;
    }
    return *this; // Faltou isso!
}

Client::~Client() {
}

int Client::getServerFd() const {
	return this->_serverFd;
}

int Client::getClientFd() const {
	return this->_clientFd;
}

std::string Client::getResponses() const {
	return this->_responses;
}

std::string Client::getRequests() const {
	return this->_requests;
}

time_t Client::getLastActivity() const {
	return this->_lastActivity;
}

void Client::appendResponse(std::string response) {
	this->_responses += response;
}

void Client::appendRequest(std::string request) {
	this->_requests += request;
}

void Client::trimResponse(int bytes_sent) {
	this->_responses = _responses.substr(bytes_sent);
}

void Client::updateActivity() {
	this->_lastActivity = std::time(NULL);
}