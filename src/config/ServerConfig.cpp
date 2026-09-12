#include "../../includes/config/ServerConfig.hpp"

ServerConfig::ServerConfig() : interface_("0.0.0.0"), port_(0), serverName_(""), clientMaxBodySize_(""),locations_(), errorPages_() {}

ServerConfig::ServerConfig(const ServerConfig& other)
        : interface_(other.interface_),
            port_(other.port_),
      serverName_(other.serverName_),
      clientMaxBodySize_(other.clientMaxBodySize_),
      locations_(other.locations_),
      errorPages_(other.errorPages_)
{

}

ServerConfig& ServerConfig::operator=(const ServerConfig& other)
{
    if (this != &other)
    {
        interface_ = other.interface_;
        port_ = other.port_;
        serverName_ = other.serverName_;
        clientMaxBodySize_ = other.clientMaxBodySize_;
        locations_ = other.locations_;
        errorPages_ = other.errorPages_;
    }
    return *this;
}

ServerConfig::~ServerConfig()
{

}

void ServerConfig::setPort(int port) {
    port_ = port;
}

int ServerConfig::getPort() const {
    return port_;
}

void ServerConfig::setInterface(const std::string& interfaceName) {
    interface_ = interfaceName;
}

const std::string& ServerConfig::getInterface() const {
    return interface_;
}

void ServerConfig::setServerName(const std::string& serverName){
    serverName_ = serverName;
}

const std::string& ServerConfig::getServerName() const {
    return serverName_;
}

void ServerConfig::setClientMaxBodySize(const std::string& size)
{
    clientMaxBodySize_ = size;
}

const std::string& ServerConfig::getClientMaxBodySize() const
{
    return clientMaxBodySize_;
}

void ServerConfig::addLocation(const LocationConfig& location)
{
    locations_.push_back(location);
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const
{
    return locations_;
}

void ServerConfig::setErrorPage(int errorCode, const std::string& page)
{
    errorPages_[errorCode] = page;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const
{
    return errorPages_;
}

std::string ServerConfig::getErrorPage(int errorCode) const
{
    std::map<int, std::string>::const_iterator it = errorPages_.find(errorCode);
    if (it != errorPages_.end())
    {
        return it->second;
    }

    return "";
}