#include "../../includes/config/ConfigParser.hpp"
#include <stdexcept>
#include <cstdlib>
#include <cctype>

void ConfigParser::handleListen(ServerConfig& server, const std::vector<std::string>& tokens, size_t& i, bool& hasListen)
{
    if (hasListen) 
        throw std::runtime_error("Error: duplicate 'listen' directive.");
    if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        throw std::runtime_error("Error: invalid syntax in 'listen' directive.");
    const std::string& listenValue = tokens[i + 1];
    std::string interfaceName = "0.0.0.0";
    std::string portString = listenValue;
    std::string::size_type separator = listenValue.find(':');

    if (separator != std::string::npos)
    {
        if (separator == 0 || separator + 1 >= listenValue.size()
            || listenValue.find(':', separator + 1) != std::string::npos)
            throw std::runtime_error("Error: invalid interface:port in 'listen'.");
        interfaceName = listenValue.substr(0, separator);
        portString = listenValue.substr(separator + 1);
    }

    if (!isValidPort(portString))
        throw std::runtime_error("Error: invalid port '" + portString + "'.");

    server.setInterface(interfaceName);
    server.setPort(std::atoi(portString.c_str()));
    hasListen = true;
    i += 3;
}

void ConfigParser::handleErrorPage(ServerConfig& server, const std::vector<std::string>& tokens, size_t& i)
{
    if (i + 3 >= tokens.size())
    {
        throw std::runtime_error("Error: invalid syntax for 'error_page' directive.");
    }

    if (tokens[i + 3] != ";")
    {
        throw std::runtime_error("Error: expected ';' after 'error_page'.");
    }

    const std::string& codeStr = tokens[i + 1];
    const std::string& pagePath = tokens[i + 2];

    if (codeStr.empty())
        throw std::runtime_error("Error: invalid error code for 'error_page'.");

    for (std::string::size_type j = 0; j < codeStr.size(); ++j)
    {
        if (!std::isdigit(codeStr[j]))
            throw std::runtime_error("Error: error code must be numeric.");
    }

    int errorCode = std::atoi(codeStr.c_str());
    if (errorCode < 400 || errorCode > 599)
        throw std::runtime_error("Error: error code must be between 400 and 599.");

    const std::map<int, std::string>& errorPages = server.getErrorPages();
    if (errorPages.find(errorCode) != errorPages.end())
    {
        throw std::runtime_error("Error: duplicate 'error_page' directive for code " + codeStr + ".");
    }

    server.setErrorPage(errorCode, pagePath);
    i += 4;
}

void ConfigParser::handleServerName(ServerConfig& server, const std::vector<std::string>& tokens, size_t& i, bool& hasServerName)
{
    if(hasServerName)
        throw std::runtime_error("Error: duplicate 'server_name' directive.");
    if(i + 2 >= tokens.size())
        throw std::runtime_error("Error: invalid syntax for 'server_name' directive.");
    if(tokens[i + 2] != ";")
        throw std::runtime_error("Error: expected ';' after 'server_name'.");
    const std::string& serverName = tokens[i + 1];
    if (serverName.empty() || serverName == "{" || serverName == "}")
    {
        throw std::runtime_error("Error: server_name cannot be empty.");
    }
    server.setServerName(serverName);
    hasServerName = true;
    i += 3;
}

void ConfigParser::handleClientMaxBodySize(ServerConfig& server, const std::vector<std::string>& tokens, size_t& i, bool& hasClientMaxBodySize)
{
    if(hasClientMaxBodySize)
        throw std::runtime_error("Error: duplicate 'client_max_body_size' directive.");
    if(i + 2 >= tokens.size())
        throw std::runtime_error("Error: invalid syntax for 'client_max_body_size' directive.");
    if(tokens[i + 2] != ";")
        throw std::runtime_error("Error: expected ';' after 'client_max_body_size'.");
    const std::string& size = tokens[i + 1];
    if(!isValidBodySize(size))
        throw std::runtime_error("Error: invalid body size.");

    server.setClientMaxBodySize(size);
    hasClientMaxBodySize = true;
    i += 3;
}

void ConfigParser::handleLocation(ServerConfig& server, const std::vector<std::string>& tokens, size_t& i)
{
    LocationConfig location = parseLocation(tokens, i);
    const std::vector<LocationConfig>& existingLocation = server.getLocations();
    for(size_t j = 0; j < existingLocation.size(); ++j)
    {
        if(existingLocation[j].getPath() == location.getPath())
        {
            throw std::runtime_error("Error: duplicate 'location' directive for path " + location.getPath() + ".");
        }
    }
    server.addLocation(location);
}

void ConfigParser::parseRootDirective(LocationConfig& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasRoot)
{
    if (hasRoot)
        throw std::runtime_error("Error: duplicate 'root' directive.");
    if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        throw std::runtime_error("Error: expected ';' after 'root'.");
    if (tokens[i + 1].empty() || tokens[i + 1] == "{" || tokens[i + 1] == "}")
    {
        throw std::runtime_error("Error: root path cannot be empty.");
    }
    loc.setRoot(tokens[i + 1]);
    hasRoot = true;
    i += 3;
}

void ConfigParser::parseMethodsDirective(LocationConfig& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasMethod)
{
    if (hasMethod)
    {
        throw std::runtime_error("Error: duplicate 'methods' directive.");
    }
    std::vector<std::string> methods;
    size_t j = i + 1;
    while (j < tokens.size() && tokens[j] != ";")
    {
        if (tokens[j] != "GET" && tokens[j] != "POST" && tokens[j] != "DELETE")
            throw std::runtime_error("Error: invalid HTTP method " + tokens[j] + ".");
        for (size_t k = 0; k < methods.size(); ++k)
        {
            if (methods[k] == tokens[j])
                throw std::runtime_error("Error: duplicate HTTP method " + tokens[j] + ".");
        }
        methods.push_back(tokens[j]);
        ++j;
    }

    if (j >= tokens.size())
        throw std::runtime_error("Error: expected ';' after 'methods'.");
    if (methods.empty())
        throw std::runtime_error("Error: 'methods' directive cannot be empty.");
    loc.setMethods(methods);
    hasMethod = true;
    i = j + 1;
}

void ConfigParser::parseDirListingDirective(LocationConfig& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasDirListing)
{
    if (hasDirListing)
        throw std::runtime_error("Error: duplicate 'directory_listing' directive.");
    if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        throw std::runtime_error("Error: invalid syntax for 'directory_listing' directive.");
    if (tokens[i + 1] == "on")
        loc.setDirectoryListing(true);
    else if (tokens[i + 1] == "off")
        loc.setDirectoryListing(false);
    else
        throw std::runtime_error("Error: invalid value for 'directory_listing'. Use 'on' or 'off'.");
    hasDirListing = true;
    i += 3;
}

void ConfigParser::parseReturnDirective(LocationConfig& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasReturn)
{
    if (hasReturn)
        throw std::runtime_error("Error: duplicate return directive.");
    if (i + 3 >= tokens.size() || tokens[i + 3] != ";")
        throw std::runtime_error("Error: expected ';' after 'return'.");
    const std::string& codeStr = tokens[i + 1];
    if (codeStr.empty())
        throw std::runtime_error("Error: invalid redirect code.");

    for (size_t j = 0; j < codeStr.size(); ++j)
    {
        if (!std::isdigit(codeStr[j]))
            throw std::runtime_error("Error: redirect code must be numeric.");
    }

    int redirCode = std::atoi(codeStr.c_str());
    if (redirCode != 301 && redirCode != 302 && redirCode != 303 &&
        redirCode != 307 && redirCode != 308)
        throw std::runtime_error("Error: invalid redirect code.");
    loc.setRedirectCode(redirCode);
    loc.setRedirectUrl(tokens[i + 2]);
    hasReturn = true;
    i += 4;
}

void ConfigParser::parseIndexDirective(LocationConfig& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasIndex)
{
    if (hasIndex)
        throw std::runtime_error("Error: duplicate 'index' directive.");
    if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        throw std::runtime_error("Error: expected ';' after 'index'.");
    if (tokens[i + 1].empty() || tokens[i + 1] == "{" || tokens[i + 1] == "}")
    {
        throw std::runtime_error("Error: 'index' file cannot be empty.");
    }
    loc.setIndex(tokens[i + 1]);
    hasIndex = true;
    i += 3;
}

void ConfigParser::parseUploadDirective(LocationConfig& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasUpload)
{
    if (hasUpload)
        throw std::runtime_error("Error: duplicate 'upload' directive.");
    if (i + 1 >= tokens.size() || tokens[i + 1] == ";")
        throw std::runtime_error("'upload' path cannot be empty");
    if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        throw std::runtime_error("Error: expected ';' after 'upload'.");
    loc.setUploadPath(tokens[i + 1]);
    hasUpload = true;
    i += 3;
}