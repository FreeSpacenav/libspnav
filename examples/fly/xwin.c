#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include "xwin.h"

void reshape(int x, int y);
void set_window_title(const char *title);

Display *dpy;
Atom wm_prot, wm_del_win;
GLXContext ctx;
Window win;
int redisplay_pending;

int create_xwin(const char *title, int xsz, int ysz)
{
	int scr;
	Window root;
	XVisualInfo *vis;
	XSetWindowAttributes xattr;
	unsigned int events;
	XClassHint class_hint;

	int attr[] = {
		GLX_RGBA, GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		None
	};

	wm_prot = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wm_del_win = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);

	if(!(vis = glXChooseVisual(dpy, scr, attr))) {
		fprintf(stderr, "requested GLX visual is not available\n");
		return -1;
	}

	if(!(ctx = glXCreateContext(dpy, vis, 0, True))) {
		fprintf(stderr, "failed to create GLX context\n");
		XFree(vis);
		return -1;
	}

	xattr.background_pixel = xattr.border_pixel = BlackPixel(dpy, scr);
	xattr.colormap = XCreateColormap(dpy, root, vis->visual, AllocNone);

	if(!(win = XCreateWindow(dpy, root, 0, 0, xsz, ysz, 0, vis->depth, InputOutput,
					vis->visual, CWColormap | CWBackPixel | CWBorderPixel, &xattr))) {
		fprintf(stderr, "failed to create X window\n");
		return -1;
	}
	XFree(vis);

	/* set the window event mask */
	events = ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask |
		ButtonReleaseMask |	ButtonPressMask | PointerMotionMask;
	XSelectInput(dpy, win, events);

	XSetWMProtocols(dpy, win, &wm_del_win, 1);

	set_window_title(title);

	class_hint.res_name = "libspnav_example";
	class_hint.res_class = "libspnav_example";
	XSetClassHint(dpy, win, &class_hint);

	if(glXMakeCurrent(dpy, win, ctx) == False) {
		fprintf(stderr, "glXMakeCurrent failed\n");
		glXDestroyContext(dpy, ctx);
		XDestroyWindow(dpy, win);
		return -1;
	}

	XMapWindow(dpy, win);
	XFlush(dpy);

	return 0;
}

void destroy_xwin(void)
{
	glXDestroyContext(dpy, ctx);
	XDestroyWindow(dpy, win);
	glXMakeCurrent(dpy, None, 0);
}

void set_window_title(const char *title)
{
	XTextProperty wm_name;

	XStringListToTextProperty((char**)&title, 1, &wm_name);
	XSetWMName(dpy, win, &wm_name);
	XSetWMIconName(dpy, win, &wm_name);
	XFree(wm_name.value);
}


int handle_xevent(XEvent *xev)
{
	int x, y;
	KeySym sym;

	switch(xev->type) {
	case Expose:
		redisplay_pending = 1;
		break;

	case ClientMessage:
		if(xev->xclient.message_type == wm_prot) {
			if(xev->xclient.data.l[0] == wm_del_win) {
				return 1;
			}
		}
		break;

	case KeyPress:
		sym = XLookupKeysym((XKeyEvent*)&xev->xkey, 0);
		if((sym & 0xff) == 27) {
			return 1;
		}
		break;

	case ConfigureNotify:
		x = xev->xconfigure.width;
		y = xev->xconfigure.height;

		reshape(x, y);
		break;

	default:
		break;
	}
	return 0;
}
