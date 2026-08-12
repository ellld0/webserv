#include "ServerConfig.hpp"

Server::Server() : port_(0), serverName_(""), clientMaxBodySize_(""),locations_(), errorPages_() {}

void Server::setPort(int port) {
    port_ = port;
}

int Server::getPort() const {
    return port_;
}

void Server::setServerName(const std::string& serverName){
    serverName_ = serverName;
}

const std::string& Server::getServerName() const {
    return serverName_;
}

void Server::setClientMaxBodySize(const std::string& size)
{
    clientMaxBodySize_ = size;
}

const std::string& Server::getClientMaxBodySize() const
{
    return clientMaxBodySize_;
}

void Server::addLocation(const Location& location)
{
    locations_.push_back(location);
}

const std::vector<Location>& Server::getLocations() const
{
    return locations_;
}

void Server::setErrorPage(int errorCode, const std::string& page)
{
    errorPages_[errorCode] = page;
}

const std::map<int, std::string>& Server::getErrorPages() const
{
    return errorPages_;
}

std::string Server::getErrorPage(int errorCode) const
{
    std::map<int, std::string>::const_iterator it = errorPages_.find(errorCode);
    if (it != errorPages_.end())
    {
        return it->second;
    }

    return "";
}
