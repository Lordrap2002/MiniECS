CC=gcc

%.o: %.c
	$(CC) -c -o $@ $<

all: client server

client: client.o
	gcc -o client client.o
	
server: server.o
	gcc -o server server.o -pthread