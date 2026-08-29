#include "../../includes/network/ServerManager.hpp"
#include <cstring>

ServerManager::ServerManager() {}

ServerManager::ServerManager(const ServerManager& other) {
    *this = other;
}

ServerManager& ServerManager::operator=(const ServerManager& other) {
    if (this != &other) {
        this->_pollFds = other._pollFds;
        this->_serverSockets = other._serverSockets;
    }
    return *this;
}

ServerManager::~ServerManager() {
    // No futuro, faremos um loop aqui para fechar (close) todos os FDs abertos!
}

void ServerManager::init(const std::vector<ServerConfig>& configs) {
    std::cout << "[INFO] Starting server sockets..." << std::endl;
    
    for (size_t i = 0; i < configs.size(); ++i) {
        try {
            setupSocket(configs[i]);
        } catch (const std::exception& e) {
            std::cerr << "[ERRO] Falha ao configurar porta " << configs[i].getPort() 
                      << ": " << e.what() << std::endl;
            // Se um servidor falhar, podemos decidir se paramos tudo ou tentamos o próximo
        }
    }
}

void ServerManager::setupSocket(const ServerConfig& config) {
    // 1. Criar o Socket (O 'telefone' para a internet)
    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = IP (Protocolo Padrão)
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        throw std::runtime_error("Error: Failed to create Socket!");
    }

    // 2. Opção de Reuso de Porta (CRUCIAL para testes)
    // Evita o erro chato de "Address already in use" se você fechar e abrir o servidor rápido
    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverFd);
        throw std::runtime_error("Erro no setsockopt (SO_REUSEADDR)");
    }

    // 3. Transformar o Socket em NON-BLOCKING (Requisito obrigatório do poll)
    // Isso impede que o servidor "congele" esperando dados de um cliente lento
    if (fcntl(serverFd, F_SETFL, O_NONBLOCK) < 0) {
        close(serverFd);
        throw std::runtime_error("Erro to setup fcntl (O_NONBLOCK)");
    }

    // 4. Preparar o endereço e a porta (Bind)
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address)); // Zera a estrutura
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Ouve em todas as interfaces de rede (0.0.0.0)
    address.sin_port = htons(config.getPort()); // Converte a porta para o formato da rede (Big Endian)

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(serverFd);
        throw std::runtime_error("Erro no bind. Porta possivelmente em uso.");
    }

    // 5. Começar a escutar (Listen)
    // SOMAXCONN diz ao Kernel para criar a maior fila de espera possível para clientes simultâneos
    if (listen(serverFd, SOMAXCONN) < 0) {
        close(serverFd);
        throw std::runtime_error("Erro no listen");
    }

    // 6. Sucesso! Vamos registrar isso nas nossas estruturas de dados (O Poll e o Map)
    struct pollfd pfd;
    pfd.fd = serverFd;
    pfd.events = POLLIN; // Queremos ser avisados pelo poll() quando houver dados para ler (novos clientes)
    pfd.revents = 0;

    _pollFds.push_back(pfd);             // Adiciona na lista do poll
    _serverSockets[serverFd] = config;   // Salva a configuração no nosso dicionário rápido!

    std::cout << "[INFO] Socket aberto com sucesso na porta: " << config.getPort() 
              << " (FD: " << serverFd << ")" << std::endl;
}

// O motor principal
void ServerManager::run() {
    // Por enquanto, apenas para manter o programa aberto e testarmos se as portas abriram
    std::cout << "[INFO] Server listening..." << std::endl;
    while (true) {
        // (Aqui entrará o poll() na próxima etapa)
    }
}