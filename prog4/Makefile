CC=gcc
CFLAGS=-ansi -pedantic-errors -Wall

default: all	
	$(CC) -o reserver reserver.o
	$(CC) -o reclient reclient.o

all:		reserver.o reclient.o

reserver.o:	reserver.c
	$(CC) $(CFLAGS) -c reserver.c

reclient.o:	reclient.c
	$(CC) $(CFLAGS) -c reclient.c

.PHONY: clean

clean:
	rm -rf reserver reserver.o reclient reclient.o
