CC = gcc
CFLAGS = -pedantic -Wall -g -I..
LDFLAGS = -L.. -lspnav -lX11

all:	simple_x11 simple_af_unix

simple_x11:	simple.c
	$(CC) $(CFLAGS) $(LDFLAGS) -DBUILD_X11 -o $@ $^
simple_af_unix:	simple.c
	$(CC) $(CFLAGS) $(LDFLAGS) -DBUILD_AF_UNIX -o $@ $^

.PHONY: clean
clean:
	rm -f simple_x11 simple_x11.o \
	      simple_af_unix simple_af_unix.o

