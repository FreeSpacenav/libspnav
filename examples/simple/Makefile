CC = gcc
CFLAGS = -pedantic -Wall -g -I../.. -I/usr/local/include
LDFLAGS = -L../.. -L/usr/local/lib -lspnav -lX11

.PHONY: all
all: simple_x11 simple_af_unix

simple_x11: simple.c
	$(CC) $(CFLAGS) -DBUILD_X11 -o $@ $< $(LDFLAGS)
simple_af_unix: simple.c
	$(CC) $(CFLAGS) -DBUILD_AF_UNIX -o $@ $< $(LDFLAGS)

.PHONY: clean
clean:
	rm -f simple_x11 simple_af_unix
