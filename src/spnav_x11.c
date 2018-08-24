/*
This file is part of libspnav, part of the spacenav project (spacenav.sf.net)
Copyright (C) 2007-2018 John Tsiombikas <nuclear@member.fsf.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/
#ifdef USE_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "inpsrc.h"
#include "spnav_x11.h"

static int open_conn(void);
static int close_conn(void);
static int pending(void);
static int process(void);
static int set_flt(double val);
static int get_flt(double *res);

static int set_xwindow(Window win);
static Window get_daemon_window(Display *dpy);
static int catch_badwin(Display *dpy, XErrorEvent *err);

static struct input_src inpsrc;
static Display *dpy;
static Window app_win;
static Atom motion_event, button_press_event, button_release_event, command_event;

enum {
	CMD_APP_WINDOW = 27695,
	CMD_APP_SENS
};


int init_spnav_x11(void)
{
	inpsrc.name = "spnav-x11";
	inpsrc.fd = -1;
	inpsrc.data = 0;
	inpsrc.disp = 0;

	inpsrc.open = open_conn;
	inpsrc.close = close_conn;
	inpsrc.pending = pending;
	inpsrc.process = process;
	inpsrc.set_flt = set_flt;
	inpsrc.get_flt = get_flt;

	return 0;
}


static int open_conn(void)
{
	struct x11_data *xdata = inpsrc.data;
	if(!xdata) {
		return -1;
	}

	dpy = xdata->dpy;
	app_win = xdata->win;

	motion_event = XInternAtom(dpy, "MotionEvent", True);
	button_press_event = XInternAtom(dpy, "ButtonPressEvent", True);
	button_release_event = XInternAtom(dpy, "ButtonReleaseEvent", True);
	command_event = XInternAtom(dpy, "CommandEvent", True);

	if(!motion_event || !button_press_event || !button_release_event || !command_event) {
		dpy = 0;
		app_win = 0;
		return -1;	/* daemon not started */
	}

	if(set_xwindow(app_win) == -1) {
		dpy = 0;
		app_win = 0;
		return -1;	/* daemon not started */
	}
	return 0;
}

static int close_conn(void)
{
	if(!dpy) return -1;

	set_xwindow(DefaultRootWindow(dpy));
	app_win = 0;
	dpy = 0;
	return 0;
}

static int pending(void);
static int process(void);
static int set_flt(double val);
static int get_flt(double *res);

static int set_xwindow(Window win)
{
	int (*prev_xerr_handler)(Display*, XErrorEvent*);
	XEvent xev;
	Window daemon_win;

	if(!dpy) return -1;

	if(!(daemon_win = get_daemon_window(dpy))) {
		return -1;
	}

	prev_xerr_handler = XSetErrorHandler(catch_badwin);

	xev.type = ClientMessage;
	xev.xclient.send_event = False;
	xev.xclient.display = dpy;
	xev.xclient.window = win;
	xev.xclient.message_type = command_event;
	xev.xclient.format = 16;
	xev.xclient.data.s[0] = ((unsigned int)win & 0xffff0000) >> 16;
	xev.xclient.data.s[1] = (unsigned int)win & 0xffff;
	xev.xclient.data.s[2] = CMD_APP_WINDOW;

	XSendEvent(dpy, daemon_win, False, 0, &xev);
	XSync(dpy, False);

	XSetErrorHandler(prev_xerr_handler);
	return 0;
}

#else
/* libspnav built without X11 support */

int init_spnav_x11(void)
{
	return -1;
}
#endif
