# Guia de Trabalho

### 🗂️ Fase 1: Módulo de Configuração (Dev 1)
Estes são os "Models" do sistema. Eles guardam os dados estáticos lidos do arquivo .conf.

1.  **ConfigParser.hpp**
    *   **Propósito:** É a classe responsável por abrir o arquivo e transformar o texto em objetos.
    *   **O Contrato Principal:** `std::vector<ServerConfig> parseFile(const std::string& filename);`
    *   *Quem usa:* O `main.cpp` no início do programa para gerar a lista de configurações.

2.  **ServerConfig.hpp**
    *   **Propósito:** Representa um bloco `server {}` inteiro. Guarda a porta, o nome do servidor, o limite de corpo e uma lista de rotas.
    *   **O Contrato Principal:** Métodos como `int getPort() const;`, `int getClientMaxBodySize() const;` e `const LocationConfig* matchLocation(const std::string& path) const;` (que devolve a regra específica para a rota solicitada).

3.  **LocationConfig.hpp**
    *   **Propósito:** Representa um bloco `location {}`. Define regras rígidas para um diretório específico.
    *   **O Contrato Principal:** Métodos como `bool isMethodAllowed(std::string method) const;`, `bool isAutoIndexOn() const;`, `std::string getRoot() const;`.
    *   *Quem usa:* O Dev 3 (HTTP), o tempo todo, para saber se tem permissão para realizar a ação pedida pelo cliente.

### ⚙️ Fase 2: Módulo de Rede e Loop (Dev 2)
Estes são os "motores" do sistema. Eles gerenciam os descritores de arquivo e a comunicação bruta com o sistema operacional.

4.  **ServerManager.hpp (ou Webserv.hpp)**
    *   **Propósito:** Orquestrar os sockets e rodar o loop infinito do `poll()` (ou equivalente).
    *   **O Contrato Principal:** `void setupServers(const std::vector<ServerConfig>& configs);` e `void run();`.
    *   *Quem usa:* O `main.cpp`. É ele quem mantém o programa vivo.

5.  **Client.hpp**
    *   **Propósito:** Representa um navegador conectado. Guarda o ID do socket (FD), a string bruta que está chegando aos poucos e o estado da resposta.
    *   **O Contrato Principal:** Métodos como `void appendRawData(const std::string& data);`, `std::string getRawData() const;` e métodos para checar se a conexão deve ser fechada.
    *   *Quem usa:* O próprio Dev 2 para gerenciar o estado daquele cliente específico dentro do `poll()`.

### 🧠 Fase 3: Módulo HTTP e CGI (Dev 3)
Estes são os "Controllers". Eles pegam dados brutos, aplicam a lógica de negócio e geram o resultado final.

6.  **Request.hpp**
    *   **Propósito:** Transformar a string bruta que o Dev 2 leu do socket em variáveis estruturadas.
    *   **O Contrato Principal:** `bool parse(const std::string& raw_buffer);`, `std::string getMethod() const;`, `std::string getPath() const;`, `std::string getHeader(const std::string& key) const;`.
    *   *Quem usa:* O Dev 2 passa o buffer para cá, e o Dev 3 usa os getters para entender o pedido.

7.  **Response.hpp**
    *   **Propósito:** Pegar as informações do `Request` e do `LocationConfig`, processar o que foi pedido (como ler um HTML ou gerar um erro) e montar a string de resposta.
    *   **O Contrato Principal:** `void build(const Request& req, const ServerConfig& config);`, `std::string toString() const;` (que devolve o "HTTP/1.1 200 OK..." pronto para ser enviado).
    *   *Quem usa:* O Dev 2 chama `toString()` para enviar a resposta pelo socket via chamadas de escrita.

8.  **CgiHandler.hpp**
    *   **Propósito:** Isolar a sujeira de criar processos filhos com `fork`, gerenciar pipes e executar `execve` para rodar scripts (como o PHP-CGI).
    *   **O Contrato Principal:** `std::string execute(const Request& req, const std::string& script_path);`.
    *   *Quem usa:* O próprio `Response.hpp` aciona o CGI internamente quando percebe que a rota pedida exige a execução de um script.

### 🛠️ Compartilhados
Para evitar conflitos e arquivos gigantes, é bom ter alguns arquivos auxiliares:

9.  **HttpData.hpp ou Enums.hpp**
    *   Para guardar constantes úteis que os três vão usar. Por exemplo: códigos de status de resposta (200, 404, 500, etc.) e enumerações de estado do cliente (`READING`, `WRITING`, `DISCONNECTED`).