CC = gcc
CFLAGS = -Wall

all: main

main: json_parser.o
	$(CC) $(CFLAGS) json_parser.o main.c -o main

json_parser.o: json_parser.c json_parser.h
	$(CC) $(CFLAGS) json_parser.c -c -o json_parser.o

clean:
	rm -f *.o main
