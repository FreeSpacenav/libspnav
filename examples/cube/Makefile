obj = cube.o vmath.o
bin = cube

CC = gcc
CFLAGS = -pedantic -Wall -g -I../..
LDFLAGS = -L../.. -lX11 -lGL -lGLU -lm -lspnav

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
