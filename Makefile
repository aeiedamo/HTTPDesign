
all: main

main: main.o
	gcc -pthread -o main main.c -Wall -g

