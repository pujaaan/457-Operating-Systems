#Make file for A1

CC=gcc
CFLAGS=-Wall
LDFLAGS=-lssl -lcrypto

all: gls

gls: gls.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
clean: 
	rm -f a1
