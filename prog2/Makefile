CC=gcc
CFLAGS=-ansi -pedantic-errors -Wall

default: all	
	$(CC) -o simplesh simplesh.o

all:		simplesh.o

simplesh.o:	simplesh.c
	$(CC) $(CFLAGS) -c simplesh.c

.PHONY: clean

clean:
	rm -rf simplesh simplesh.o
