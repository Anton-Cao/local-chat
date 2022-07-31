all: server client

server: src/server.c
	clang -o build/server src/server.c

client: src/client.c
	clang -o build/client src/client.c
