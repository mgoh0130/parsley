# NAME: Michelle Goh
#   NetId: mg2657
CC=gcc
CFLAGS= -std=c99 -pedantic -Wall -g3 -I/c/cs323/Hwk2/

parsley: parsley.o /c/cs323/Hwk2/mainParsley.o
		${CC} ${CFLAGS} $^ -o $@

parsley.o: /c/cs323/Hwk2/parsley.h

clean:
		rm -f parsley *.o
