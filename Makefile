all: mecs
mecs: main
	gcc -o mecs main.o
main: main.c
	gcc -c main.c