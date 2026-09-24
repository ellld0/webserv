#include "../../includes/http/Request.hpp"

Request::Request() : _complete(false) {}
Request::~Request() {}

Request::Request(const Request &other)
{
    (void)other;
}

Request &Request::operator=(const Request &other)
{
    (void)other;
    return *this;
}

void Request::_parseRequestLine(const std::string& line)
{
    std::istringstream ss(line);
    ss >> _method;
    ss >> _path;
    ss >> _httpVersion;

    size_t queryPos = _path.find('?');

    if (queryPos != std::string::npos)
    {
        _queryString = _path.substr(queryPos + 1);
        _path = _path.substr(0, queryPos);
    }
}

void Request::_parseHeaders(const std::string& rawHeaders)
{
    std::istringstream ss(rawHeaders);
    std::string line;

    while (std::getline(ss, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (line.empty())
            continue;

        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;

        std::string headerName = line.substr(0, colonPos);
        std::string headerValue;
        if (colonPos + 1 < line.size())
            headerValue = line.substr(colonPos + 1);

        while (!headerValue.empty() && (headerValue[0] == ' ' || headerValue[0] == '\t'))
            headerValue.erase(0, 1);

        _headers[headerName] = headerValue;
    }
}

// Método publico principal para parsear a requisição HTTP

bool Request::parse(const std::string& rawBuffer)
{
    std::string separator = "\r\n\r\n";
    size_t headerEndPos = rawBuffer.find(separator);

    if (headerEndPos == std::string::npos) {
        _complete = false;
        return false;
    }

    // 1. Calcula o tamanho que já chegou SEM ALOCAR NADA NA MEMÓRIA! (Super Rápido)
    size_t current_body_length = rawBuffer.length() - (headerEndPos + separator.size());

    // 2. Extrai APENAS o cabeçalho para fazer o parse (é pequeno, rápido)
    std::string headerSection = rawBuffer.substr(0, headerEndPos);

    std::istringstream ss(headerSection);
    std::string requestLine;
    std::getline(ss, requestLine);

    if (!requestLine.empty() && requestLine[requestLine.size() - 1] == '\r')
        requestLine.erase(requestLine.size() - 1);

    _parseRequestLine(requestLine);

    std::string headersRaw = headerSection.substr(requestLine.size() + 2);
    
    // PRIMEIRO parseia e salva os cabeçalhos (para o getHeader funcionar abaixo)
    _parseHeaders(headersRaw);

    // 3. A Trava Matemática
    // Lemos o Transfer-Encoding aqui fora para a variável sobreviver até o Passo 4
    std::string tEncStr = getHeader("Transfer-Encoding");
    if (tEncStr.empty()) tEncStr = getHeader("transfer-encoding");

    if (_method == "POST") {
        std::string cLenStr = getHeader("Content-Length");
        if (cLenStr.empty()) cLenStr = getHeader("Content-length");
        if (cLenStr.empty()) cLenStr = getHeader("content-length");
        
        if (!cLenStr.empty()) {
            size_t expected_length = std::strtoul(cLenStr.c_str(), NULL, 10);
            
            // Rejeita antes de fazer o substr() pesado se ainda não chegou tudo!
            if (current_body_length < expected_length) {
                _complete = false;
                return false;
            }
        }
        else if (tEncStr.find("chunked") != std::string::npos) {
            // Modo chunked: Procura no rawBuffer porque o _body ainda não foi alocado!
            if (rawBuffer.find("0\r\n\r\n") == std::string::npos) {
                _complete = false;
                return false;
            }
        }
    }

    // 4. Se chegou aqui, os 100MB chegaram inteiros!
    // Vamos "descompactar" se for chunked, ou copiar direto se for tamanho fixo
    if (tEncStr.find("chunked") != std::string::npos) {
        // Descompacta o Chunked Body sem travar o C++ (Otimizado)
        size_t pos = headerEndPos + separator.size();
        _body.reserve(rawBuffer.length()); // Evita cópias lentas na memória
        
        while (pos < rawBuffer.length()) {
            size_t lineEnd = rawBuffer.find("\r\n", pos);
            if (lineEnd == std::string::npos) break;
            
            // Pega o tamanho em hexadecimal e converte pra decimal (base 16)
            std::string hexStr = rawBuffer.substr(pos, lineEnd - pos);
            size_t chunkSize = std::strtoul(hexStr.c_str(), NULL, 16);
            
            if (chunkSize == 0) break; // 0\r\n\r\n indica o fim do envio
            
            pos = lineEnd + 2; // Pula o hex e o \r\n
            _body.append(rawBuffer, pos, chunkSize); // Extrai só os dados puros
            pos += chunkSize + 2; // Pula os dados e o \r\n do final do chunk
        }
    } else {
        // Modo normal: Faz a cópia gigante UMA ÚNICA VEZ.
        _body = rawBuffer.substr(headerEndPos + separator.size());
    }
    
    _complete = true;
    return true;
}

bool Request::isComplete() const { return _complete; }

std::string Request::getMethod() const { return _method; }

std::string Request::getPath() const { return _path; }

std::string Request::getQueryString() const { return _queryString; }

std::string Request::getHttpVersion() const { return _httpVersion; }

const std::string& Request::getBody() const { return _body; }

std::string Request::getHeader(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    
    if (it == _headers.end())
        return "";

    return it->second;
}