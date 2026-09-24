*This project has been created as part of the 42 curriculum by bnanque, gabriede, makamins*

# Webserv

## Description

Webserv is an HTTP server written in C++98. The project provides a non-blocking server based on poll() and supports configurable listening ports, static files, HTTP methods, custom error pages, uploads, redirects, and CGI integration.

The configuration follows a structure inspired by the NGINX server and location blocks. Multiple server blocks can be declared in the same configuration file.

### Configuration and parser

The configuration parser is responsible for:

- Reading and validating `server` and `location` blocks.
- Validating listening ports, including `interface:port` values such as `127.0.0.1:9080`.
- Storing the listening interface and port in `ServerConfig` for the network layer.
- Validating accepted methods: `GET`, `POST`, and `DELETE`.
- Parsing client body limits, error pages, roots, index files, directory listing, uploads, and redirects.
- Detecting duplicate directives and malformed configuration blocks.

## Instructions

### Requirements

- A Unix-like operating system.
- A C++ compiler with C++98 support.
- `make`.

### Compilation

From the repository root:

```bash
make
```

The Makefile uses:

```text
-std=c++98 -Wall -Wextra -Werror
```

To remove object files:

```bash
make clean
```

To remove all generated files:

```bash
make fclean
```

To rebuild from scratch:

```bash
make re
```

### Execution

Run the server with a configuration file:

```bash
./webserv conf/default.conf
```

The server also uses `conf/default.conf` when started without an argument:

```bash
./webserv
```

A custom listening address can be written as:

```nginx
server {
    listen 127.0.0.1:9080;
    server_name localhost;
    client_max_body_size 10M;

    location / {
        root ./www;
        methods GET;
        index index.html;
        directory_listing on;
    }
}
```

The current sample configuration demonstrates multiple listening ports, static content, custom error pages, uploads, redirects, and per-location settings.

## Resources

### References

**Linux Manual** (https://man7.org/index.html ) - To understand every syscall functions used is this project like poll(), send(), accept() and others. Also to understand struct data used via libraries like pollfd and sockaddr.

**CPP Reference** (https://cppreference.com/) - To understand, discovery methods and features from native classes from C++98 utilized in project like std::map and std::vector. 

**IBM Documentation** (https://www.ibm.com/docs/en/i/7.4.0?topic=designs-using-poll-instead-select) - How poll() works, why poll() between select() and the base of an webserver code.
 
**Youtube** (https://www.youtube.com/watch?v=YwHErWJIh6Y&t=684s) - C++ Web Server from Scratch | Part 1: Creating a Socket Object by Eric O Meehan - Learn how to build an webserver in C++

**IETF Org** (https://datatracker.ietf.org/doc/html/rfc3875) - To understand what CGI is and how it works.

**Alphabetical Index of Directives** — https://nginx.org/en/docs/dirindex.html
Used to understand how to implement the configuration file, inspired by the Nginx configuration structure.

**Beginner’s Guide** — https://nginx.org/en/docs/beginners_guide.html
A detailed guide to understanding Nginx and its configuration system.

### Use of artificial intelligence

Artificial intelligence was used as a development and review assistant. It helped with:
 - Frontend construction
 - Research about how some functions works
 - Spot potential errors

The final design decisions, code integration, testing, and responsibility for the submitted implementation remain with the project team.

## Project structure

```text
conf/       Configuration files
includes/   C++ headers
src/        C++98 implementation
www/        Static website and error pages
Makefile    Build rules
```
