CC=gcc
CFLAGS=-ansi -pedantic-errors -Wall

default: all	
	$(CC) -o dealer dealer.o
	$(CC) -o player player.o

all:		dealer.o player.o

dealer.o:	dealer.c
	$(CC) $(CFLAGS) -c dealer.c

player.o:	player.c
	$(CC) $(CFLAGS) -c player.c

.PHONY: clean

clean:
	rm -rf dealer dealer.o player player.o
