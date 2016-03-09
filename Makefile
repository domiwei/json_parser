all: main

main: json_parser.o
	gcc json_parser.o main.c -o main -Wall

json_parser.o: json_parser.c json_parser.h
	gcc json_parser.c -c -o json_parser.o -Wall

clean:
	rm *.o main
