CC=gcc
CFLAGS=-ansi -pedantic-errors -Wall

default: all	
	$(CC) -o cserver cserver.o
	$(CC) -o cclient cclient.o

all:		cserver.o cclient.o

cserver.o:	cserver.c
	$(CC) $(CFLAGS) -c cserver.c

cclient.o:	cclient.c
	$(CC) $(CFLAGS) -c cclient.c

.PHONY: clean

clean:
	rm -rf cserver cserver.o cclient cclient.o
