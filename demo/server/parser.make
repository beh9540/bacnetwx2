CC= gcc
CFLAGS= -I -g -Wall

SRC= parser.c
OBJ= parser.o

$(OBJ): $(SRC)

parser: $(OBJ) testParser.c

clean:
	rm -f parser.o testParser
