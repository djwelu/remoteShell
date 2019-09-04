all: server client

server: server.c
	gcc -lpthread server.c -Wall -o serverside

client: client.c
	gcc client.c -Wall -o clientside
