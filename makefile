fitch: main.c lexer.c parser.c lexer.h parser.h
	cc -o fitch lexer.c parser.c main.c -pedantic -Wall -std=c99