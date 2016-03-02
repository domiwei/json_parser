all: json_parser

json_parser: json_parser.c json_parser.h
	gcc json_parser.c -o json_parser -Wall

clean:
	rm json_parser
