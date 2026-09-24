# Commands to Test Web Serv

## Create 100MB File
dd if=/dev/zero of=arquivo_100mb.txt bs=1M count=100

## Test POST 100MB
curl -v -X POST --data-binary @arquivo_100mb.txt http://localhost:9080/directory/youpi.bla

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

## Siege Test
siege -b -c 100 -t 30S http://localhost:9080


## Tester

### Create requested directorys
`bash from source

mkdir YoupiBanane \
&& touch YoupiBanane/youpi.bad_extension \
&& touch YoupiBanane/youpi.bla \
&& mkdir YoupiBanane/nop \
&& touch YoupiBanane/nop/youpi.bad_extension \
&& touch YoupiBanane/nop/other.pouic \
&& mkdir YoupiBanane/Yeah \
&& touch YoupiBanane/Yeah/not_happy.bad_extension


### Copy cgi_tester to Root
chmod +x cgi_tester