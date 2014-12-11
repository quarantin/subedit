LIB=-lm
CFLAGS=-ggdb -pedantic -std=c99 -Wall
#CFLAGS=-ggdb -pedantic -Wall
OUTPUT=subedit
SRC=subedit.c

all:
	gcc ${SRC} -o ${OUTPUT} ${CFLAGS} ${LIB} 

clean:
	rm -f subedit; rm -f *.bkp.*;

