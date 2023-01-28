obj = cube.o xwin.o
bin = cube

CC ?= gcc
CFLAGS = -pedantic -Wall -g -I../.. -I../../src -I/usr/local/include
LDFLAGS = -L../.. -L/usr/local/lib -lX11 -lGL -lGLU -lspnav -lm

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
