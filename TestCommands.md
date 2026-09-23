# Commands to Test Web Serv

## GET
curl -i -X GET http://localhost:9080

## UPLOAD Test
curl -i -X POST --data-binary @arquivo_teste.txt http://localhost:9080/uploads?filename=arquivo_teste.txt
curl -i -X POST -d "12345" http://localhost:9080/uploads?filename=count.txt

## GET File Test
curl -i -X GET http://localhost:9080/uploads/count.txt
curl -i -X GET http://localhost:9080/uploads/arquivo_teste.txt

## DELETE Test
curl -i -X DELETE http://localhost:9080/uploads?filename=arquivo_teste.txt
curl -i -X DELETE http://localhost:9080/uploads?filename=count.txt

## UNKNOW Test	
curl -i -X BATATA_ASSADA http://localhost:9080/

## Phyton script tests

### GET
curl -i "http://localhost:9080/cgi-bin/script.py?aluno=aprovado"

### POST
curl -i -X POST -d "mensagem=isso_eh_um_post" "http://localhost:9080/cgi-bin/script.py"

### Error Script
curl -i "http://localhost:9080/cgi-bin/error_script.py"

### Infinity Loop
curl -i "http://localhost:9080/cgi-bin/infinity_loop.py"