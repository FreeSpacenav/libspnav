obj = cube.o
bin = cube

CC = gcc
CFLAGS = -pedantic -Wall -g -I../.. -I../../src -I/usr/local/include
LDFLAGS = -L../.. -L/usr/local/lib -lGL -lGLU -lspnav -lX11 -lm

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
