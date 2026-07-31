webserv/
├── Makefile                 # Arquivo obrigatório com regras NAME, all, clean, fclean e re 
├── README.md                # Obrigatório na raiz, com Descrição, Instruções e Recursos 
├── conf/                    # Pasta para guardar seus arquivos de configuração
│   └── default.conf         # Arquivo de configuração de exemplo para testar 
├── www/                     # O diretório raiz onde ficará o seu "site" falso
│   ├── index.html           # Página estática principal
│   ├── 404.html             # Páginas de erro padrão 
│   ├── uploads/             # Pasta autorizada para o envio de arquivos dos clientes 
│   └── cgi-bin/             # Scripts CGI (como arquivos .php ou .py) para testar a execução 
├── inc/                     # Todos os arquivos de cabeçalho (.hpp) 
│   ├── config/              # Contratos do Dev 1 (Fase 1)
│   │   ├── ConfigParser.hpp
│   │   ├── ServerConfig.hpp
│   │   └── LocationConfig.hpp
│   ├── network/             # Contratos do Dev 2 - SUA PASTA (Fase 2)
│   │   ├── ServerManager.hpp
│   │   └── Client.hpp
│   ├── http/                # Contratos do Dev 3 (Fase 3)
│   │   ├── Request.hpp
│   │   ├── Response.hpp
│   │   └── CgiHandler.hpp
│   └── utils/               # Utilitários globais (Enums, constantes, etc.)
│       └── WebServ.hpp
└── src/                     # Todos os arquivos de código-fonte (.cpp) 
    ├── main.cpp             # O ponto de entrada do programa
    ├── config/              # Implementações do Dev 1
    │   └── ... (.cpp)
    ├── network/             # Implementações do Dev 2 - SUA PASTA
    │   └── ... (.cpp)
    ├── http/                # Implementações do Dev 3
    │   └── ... (.cpp)
    └── utils/               # Implementações utilitárias
        └── ... (.cpp)