CC=gcc

%.o: %.c
	$(CC) -c -o $@ $<

all: mecs

mecs: main
	gcc -o mecs main.o
main: main.c
	gcc -c main.c

client: client.o
	gcc -o client client.o
	
server: server.o
	gcc -o server server.o