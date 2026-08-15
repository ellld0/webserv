#include <iostream>
#include "ConfigParser.hpp"
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cctype>


ConfigParser::ConfigParser()
{

}

ConfigParser::ConfigParser(const ConfigParser& other)
{
    (void)other;
}

ConfigParser& ConfigParser::operator=(const ConfigParser& other)
{
    (void)other;
    return *this;
}

ConfigParser::~ConfigParser()
{

}

    bool ConfigParser::isValidPort(const std::string& portString)
    {
        if (portString.empty())
        {
            return false;
        }

        for (std::string::size_type i = 0; i < portString.size(); ++i)
        {
            if (!std::isdigit(portString[i]))
            {
                return false;
            }
        }
         int port = std::atoi(portString.c_str());
        if (port < 1 || port > 65535)
        {
            return false;
        }
        return true;
    }

    bool ConfigParser::isValidBodySize(const std::string& sizeString)
    {
        if (sizeString.empty())
        {
            return false;
        }
        if(sizeString.size() < 2)
        {
            return false;
        }
        char lastChar = sizeString[sizeString.size() - 1];
        if (lastChar != 'K' && lastChar != 'M' && lastChar != 'G')
        {
            return false;
        }

        for(std::string::size_type i = 0; i < sizeString.size() - 1; ++i)
        {
            if(!std::isdigit(sizeString[i]))
            {
                return false;
            }
        }

        return true;
    }

    LocationConfig ConfigParser::parseLocation(const std::vector<std::string>& tokens, size_t& i)
    {
       if (i + 2 >= tokens.size() || tokens[i] != "location" || tokens[i + 2] != "{")
            throw std::runtime_error("Error: invalid syntax in 'location' block header.");
        LocationConfig location;
        location.setPath(tokens[i + 1]);
        i += 3;
        bool blockClose = false;
        bool hasRoot = false, hasMethod = false, hasDirListing = false;
        bool hasIndex = false, hasUpload = false, hasReturn = false;
        while (i < tokens.size())
        {
            const std::string& token = tokens[i];

            if (token == "root")
                parseRootDirective(location, tokens, i, hasRoot);
            else if (token == "methods")
                parseMethodsDirective(location, tokens, i, hasMethod);
            else if (token == "directory_listing")
                parseDirListingDirective(location, tokens, i, hasDirListing);
            else if (token == "return")
                parseReturnDirective(location, tokens, i, hasReturn);
            else if (token == "index")
                parseIndexDirective(location, tokens, i, hasIndex);
            else if (token == "upload")
                parseUploadDirective(location, tokens, i, hasUpload);
            else if (token == "}")
            {
                blockClose = true;
                ++i;
                break;
            }
            else
                throw std::runtime_error("Error: unknown directive in 'location': " + token + ".");
        }

        if (!blockClose)
            throw std::runtime_error("Error: location block is not closed with '}'.");

        return location;
    }
    
ServerConfig ConfigParser::parseServer(const std::vector<std::string>& tokens)
{
    if (tokens.size() < 3 || tokens[0] != "server" || tokens[1] != "{")
        throw std::runtime_error("Error: invalid 'server' block format.");
    ServerConfig server;
    size_t i = 2;
    bool blockClose = false;
    bool hasListen = false, hasServerName = false, hasMaxBody = false;
    while (i < tokens.size())
    {
       const std::string& token = tokens[i];

       if (token == "listen")
            handleListen(server, tokens, i, hasListen);
        else if (token == "server_name")
            handleServerName(server, tokens, i, hasServerName);
        else if (token == "client_max_body_size")
            handleClientMaxBodySize(server, tokens, i, hasMaxBody);
        else if (token == "location")
            handleLocation(server, tokens, i);
        else if (token == "error_page")
            handleErrorPage(server, tokens, i);
        else if (token == "}")
        {
            blockClose = true;
            ++i;
            break;
        }
        else
            throw std::runtime_error("Error: unknown directive '" + token + "'.");
    }

    if (!blockClose)
        throw std::runtime_error("Error: server block is not closed with '}'.");

    if (i < tokens.size())
        throw std::runtime_error("Error: extra tokens after closing 'server' block.");

    return server;
}

std::string ConfigParser::preprocess(const std::string& line)
{
    std::string result;
    for (std::string::size_type j = 0; j < line.size(); ++j)
    {
        char c = line[j];
        if (c == '#')
            break;
        if (c == '\t')
            c = ' ';
        if (c == '{' || c == '}' || c == ';')
        {
            result += ' ';
            result += c;
            result += ' ';
        }
        else
        {
            result += c;
        }
    }
    return result;
}

ServerConfig ConfigParser::parseConfigFile(const std::string& filename)
{
    std::vector<std::string> tokens;
    std::ifstream file(filename.c_str());

    if (!file.is_open())
    {
        throw std::runtime_error("Error opening configuration file: " + filename);
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::string linhaprocessada = preprocess(line);
        std::stringstream linha_stream(linhaprocessada);
        std::string palavra;
        while (linha_stream >> palavra)
        {
            tokens.push_back(palavra);
        }
    }

//    for (size_t i = 0; i < tokens.size(); ++i)
//    {
//        std::cout << "Token " << i << ": " << tokens[i] << std::endl;
//    }

    ServerConfig server = parseServer(tokens);
    return server;
}
