#!/usr/bin/env python3
import os
import sys

# O CGI exige que o script imprima os cabeçalhos HTTP básicos primeiro
print("Content-Type: text/html\r\n\r\n")

print("<h1>CGI Teste OK</h1>")

# Teste de GET (Variável de Ambiente)
query = os.environ.get("QUERY_STRING", "Nenhuma query")
print(f"<p>QUERY_STRING (GET): {query}</p>")

# Teste de POST (Lendo da entrada padrão - STDIN)
body = sys.stdin.read()
print(f"<p>BODY (POST): {body}</p>")

# Teste de Caminho Relativo
try:
    # Ele tenta abrir o txt apenas com o nome, sem o caminho completo.
    # Só vai funcionar se o C++ executou o CGI de dentro da pasta cgi-bin.
    with open("segredo.txt", "r") as f:
        print(f"<p>Arquivo lido: {f.read()}</p>")
except Exception as e:
    print(f"<p>Erro no caminho relativo: {e}</p>")