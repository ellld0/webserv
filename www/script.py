#!/usr/bin/env python3
import os

# 1. Os cabeçalhos obrigatórios seguidos de uma linha em branco (\r\n\r\n)
print("Content-Type: text/html\r\n")

# 2. O Corpo (HTML)
print("<!DOCTYPE html>")
print("<html>")
print("<head><title>Teste CGI</title></head>")
print("<body>")
print("<h1>Sucesso! O CGI está vivo!</h1>")

# Vamos imprimir uma variável de ambiente para provar que o _buildEnvp da Dev3 funcionou!
method = os.environ.get("REQUEST_METHOD", "Desconhecido")
print(f"<p>O método HTTP usado foi: <strong>{method}</strong></p>")

print("</body>")
print("</html>")