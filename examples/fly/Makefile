obj = fly.o xwin.o
bin = fly

CC = gcc
CFLAGS = -pedantic -Wall -O3 -I../.. -I../../src -I/usr/local/include
LDFLAGS = -L../.. -L/usr/local/lib -lGL -lGLU -lspnav -lX11 -lm

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
