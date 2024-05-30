incdir =  -I../.. -I../../src -I/usr/local/include -I/usr/X11R6/include \
		  -I/opt/homebrew/include
libdir = -L../.. -L/usr/local/lib -L/usr/X11R6/lib -L/opt/homebrew/lib

CFLAGS = -pedantic -Wall -g $(incdir)
LDFLAGS = $(libdir) -lspnav -lX11 -lm

.PHONY: all
all: simple_x11 simple_af_unix

simple_x11: simple.c
	$(CC) $(CFLAGS) -DBUILD_X11 -o $@ $< $(LDFLAGS)
simple_af_unix: simple.c
	$(CC) $(CFLAGS) -DBUILD_AF_UNIX -o $@ $< $(LDFLAGS)

.PHONY: clean
clean:
	rm -f simple_x11 simple_af_unix
