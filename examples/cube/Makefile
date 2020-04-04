BIN = cube
CC = gcc
CFLAGS = -pedantic -Wall -g -I../.. -I/usr/local/include
LDFLAGS = -L../.. -L/usr/local/lib -lX11 -lGL -lGLU -lm -lspnav
SRC=$(wildcard *.c)

.PHONY: all
all: $(BIN)_x11 $(BIN)_unix

$(BIN)_x11: $(SRC)
	$(CC) -o $@ $^ $(CFLAGS) -DBUILD_X11 $(LDFLAGS)

$(BIN)_unix: $(SRC)
	$(CC) -o $@ $^ $(CFLAGS) -DBUILD_UNIX $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(BIN)_unix $(BIN)_x11
