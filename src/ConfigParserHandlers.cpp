#include "ConfigParser.hpp"
#include <stdexcept>
#include <cstdlib>
#include <cctype>

void Parser::handleListen(Server& server, const std::vector<std::string>& tokens, size_t& i, bool& hasListen)
{
    if (hasListen) 
        throw std::runtime_error("Erro: diretiva 'listen' duplicada.");
    if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        throw std::runtime_error("Erro de sintaxe em 'listen'.");
    if (!isValidPort(tokens[i + 1]))
        throw std::runtime_error("Erro: porta inválida '" + tokens[i + 1] + "'.");

    server.setPort(std::atoi(tokens[i + 1].c_str()));
    hasListen = true;
    i += 3; // Consome: listen <porta> ;
}

void Parser::handleErrorPage(Server& server, const std::vector<std::string>& tokens, size_t& i)
{
    if (i + 3 >= tokens.size())
    {
        throw std::runtime_error("Erro: sintaxe inválida para 'error_page'.");
    }

    if (tokens[i + 3] != ";")
    {
        throw std::runtime_error("Erro: esperado ';' após 'error_page'.");
    }

    const std::string& codeStr = tokens[i + 1];
    const std::string& pagePath = tokens[i + 2];

    if (codeStr.empty())
        throw std::runtime_error("Erro: código de erro inválido para 'error_page'.");

    for (std::string::size_type j = 0; j < codeStr.size(); ++j)
    {
        if (!std::isdigit(static_cast<unsigned char>(codeStr[j])))
            throw std::runtime_error("Erro: código de erro deve ser numérico.");
    }

    int errorCode = std::atoi(codeStr.c_str());
    if (errorCode < 400 || errorCode > 599)
        throw std::runtime_error("Erro: código de erro deve estar entre 400 e 599.");

    const std::map<int, std::string>& errorPages = server.getErrorPages();
    if (errorPages.find(errorCode) != errorPages.end())
    {
        throw std::runtime_error("Erro: diretiva 'error_page' duplicada para o código " + codeStr + ".");
    }

    server.setErrorPage(errorCode, pagePath);
    i += 4;
}

void Parser::handleServerName(Server& server, const std::vector<std::string>& tokens, size_t& i, bool& hasServerName)
{
    if(hasServerName)
        throw std::runtime_error("Erro: diretiva 'server_name' duplicada.");
    if(i + 2 >= tokens.size())
        throw std::runtime_error("Erro: sintaxe inválida para 'server_name'.");
    if(tokens[i + 2] != ";")
        throw std::runtime_error("Erro: esperado ';' após 'server_name'.");
    const std::string& serverName = tokens[i + 1];
    server.setServerName(serverName);
    hasServerName = true;
    i += 3;
}

void Parser::handleClientMaxBodySize(Server& server, const std::vector<std::string>& tokens, size_t& i, bool& hasClientMaxBodySize)
{
    if(hasClientMaxBodySize)
        throw std::runtime_error("Erro: diretiva 'client_max_body_size' duplicada.");
    if(i + 2 >= tokens.size())
        throw std::runtime_error("Erro: sintaxe inválida para 'client_max_body_size'.");
    if(tokens[i + 2] != ";")
        throw std::runtime_error("Erro: esperado ';' após 'client_max_body_size'.");
    const std::string& size = tokens[i + 1];
    if(!isValidBodySize(size))
        throw std::runtime_error("Erro: tamanho de corpo inválido.");

    server.setClientMaxBodySize(size);
    hasClientMaxBodySize = true;
    i += 3;
}

void Parser::handleLocation(Server& server, const std::vector<std::string>& tokens, size_t& i)
{
    Location location = parseLocation(tokens, i);
    const std::vector<Location>& existingLocation = server.getLocations();
    for(size_t j = 0; j < existingLocation.size(); ++j)
    {
        if(existingLocation[j].getPath() == location.getPath())
        {
            throw std::runtime_error("Erro: diretiva 'location' duplicada para o caminho" + location.getPath() + ".");
        }
    }
    server.addLocation(location);
}

void Parser::parseRootDirective(Location& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasRoot)
{
    if (hasRoot)
        throw std::runtime_error("Erro: diretiva 'root' duplicada.");
    if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        throw std::runtime_error("Erro: esperado ';' após 'root'.");
    loc.setRoot(tokens[i + 1]);
    hasRoot = true;
    i += 3;
}

void Parser::parseMethodsDirective(Location& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasMethod)
{
    if (hasMethod)
    {
        throw std::runtime_error("Erro: diretiva 'methods' duplicada.");
    }
    std::vector<std::string> methods;
    size_t j = i + 1;
    while (j < tokens.size() && tokens[j] != ";")
    {
        if (tokens[j] != "GET" && tokens[j] != "POST" && tokens[j] != "DELETE")
            throw std::runtime_error("Erro: método HTTP inválido " + tokens[j] + ".");
        methods.push_back(tokens[j]);
        ++j;
    }

    if (j >= tokens.size())
        throw std::runtime_error("Erro: esperado ';' após 'methods'.");
    if (methods.empty())
        throw std::runtime_error("Erro: diretiva 'methods' não pode estar vazia.");
    loc.setMethods(methods);
    hasMethod = true;
    i = j + 1;
}

void Parser::parseDirListingDirective(Location& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasDirListing)
{
    if (hasDirListing)
        throw std::runtime_error("Erro: diretiva 'directory_listing' duplicada.");
    if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        throw std::runtime_error("Erro: sintaxe inválida para 'directory_listing'.");
    if (tokens[i + 1] == "on")
        loc.setDirectoryListing(true);
    else if (tokens[i + 1] == "off")
        loc.setDirectoryListing(false);
    else
        throw std::runtime_error("Erro: valor inválido para 'directory_listing'. Use 'on' ou 'off'.");
    hasDirListing = true;
    i += 3;
}

void Parser::parseReturnDirective(Location& loc, const std::vector<std::string>& tokens, size_t& i)
{
    if (i + 3 >= tokens.size() || tokens[i + 3] != ";")
        throw std::runtime_error("Erro: esperado ';' após o 'return'.");
    const std::string& codeStr = tokens[i + 1];
    if (codeStr.empty())
        throw std::runtime_error("Erro: código de redirecionamento inválido.");

    for (size_t j = 0; j < codeStr.size(); ++j)
    {
        if (!std::isdigit(static_cast<unsigned char>(codeStr[j])))
            throw std::runtime_error("Erro: código de redirecionamento deve ser numérico.");
    }

    int redirCode = std::atoi(codeStr.c_str());
    if (redirCode != 301 && redirCode != 302 && redirCode != 303 &&
        redirCode != 307 && redirCode != 308)
        throw std::runtime_error("Erro: código de redirecionamento inválido.");
    loc.setRedirectCode(redirCode);
    loc.setRedirectUrl(tokens[i + 2]);
    i += 4;
}

void Parser::parseIndexDirective(Location& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasIndex)
{
    if (hasIndex)
        throw std::runtime_error("Erro: diretiva 'index' duplicada.");
    if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        throw std::runtime_error("Erro: esperado ';' após 'index'.");
    if (tokens[i + 1].empty())
        throw std::runtime_error("Erro: arquivo 'index' não pode estar vazio.");
    loc.setIndex(tokens[i + 1]);
    hasIndex = true;
    i += 3;
}

void Parser::parseUploadDirective(Location& loc, const std::vector<std::string>& tokens, size_t& i, bool& hasUpload)
{
    if (hasUpload)
        throw std::runtime_error("Erro: diretiva 'upload' duplicada.");
    if (i + 1 >= tokens.size() || tokens[i + 1] == ";")
        throw std::runtime_error("Caminho de 'upload' não pode ser vazio");
    if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
        throw std::runtime_error("Erro: esperado ';' após 'upload'.");
    loc.setUploadPath(tokens[i + 1]);
    hasUpload = true;
    i += 3;
}