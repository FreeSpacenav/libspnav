obj = cube.o
bin = cube

incdir =  -I../.. -I../../src -I/usr/local/include -I/usr/X11R6/include \
		  -I/opt/homebrew/include
libdir = -L../.. -L/usr/local/lib -L/usr/X11R6/lib -L/opt/homebrew/lib

CFLAGS = -pedantic -Wall -g $(incdir)
LDFLAGS = $(libdir) -lGL -lGLU -lspnav -lX11 -lm

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
