CC = gcc
CFLAGS = -pedantic -Wall -g -I..
LDFLAGS = -L.. -lspnav -lX11

simple: simple.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f simple simple.o
