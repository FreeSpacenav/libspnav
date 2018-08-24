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
#include "spnav_config.h"

#ifdef USE_X11
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "spnav.h"
#include "spnav_x11.h"
#include "inpsrc.h"

static int open_conn(void);
static int close_conn(void);
static int pending(void);
static int process(void);
static int dropev(int type);
static int set_flt(double val);
static int get_flt(double *res);

static int x11_sensitivity(double sens);
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


int spnav_x11_init(void)
{
	inpsrc.name = "spnav-x11";
	inpsrc.fd = -1;
	inpsrc.data = 0;
	inpsrc.disp = 0;

	inpsrc.open = open_conn;
	inpsrc.close = close_conn;
	inpsrc.pending = pending;
	inpsrc.process = process;
	inpsrc.dropev = dropev;
	inpsrc.set_flt = set_flt;
	inpsrc.get_flt = get_flt;

	return 0;
}

int spnav_x11_window(Window win)
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

int spnav_x11_event(const XEvent *xev, spnav_event *event)
{
	int i;
	int xmsg_type;

	if(xev->type != ClientMessage) {
		return 0;
	}

	xmsg_type = xev->xclient.message_type;

	if(xmsg_type != motion_event && xmsg_type != button_press_event &&
			xmsg_type != button_release_event) {
		return 0;
	}

	if(xmsg_type == motion_event) {
		event->type = SPNAV_EVENT_MOTION;
		event->motion.data = &event->motion.x;

		for(i=0; i<6; i++) {
			event->motion.data[i] = xev->xclient.data.s[i + 2];
		}
		event->motion.period = xev->xclient.data.s[8];
	} else {
		event->type = SPNAV_EVENT_BUTTON;
		event->button.press = xmsg_type == button_press_event ? 1 : 0;
		event->button.bnum = xev->xclient.data.s[2];
	}
	return event->type;
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

	if(spnav_x11_window(app_win) == -1) {
		dpy = 0;
		app_win = 0;
		return -1;	/* daemon not started */
	}
	return 0;
}

static int close_conn(void)
{
	if(!dpy) return -1;

	spnav_x11_window(DefaultRootWindow(dpy));
	app_win = 0;
	dpy = 0;
	return 0;
}

static int pending(void)
{
}

static int process(void)
{
}

static Bool match_events(Display *dpy, XEvent *xev, char *arg)
{
	int evtype = *(int*)arg;

	if(xev->type != ClientMessage) {
		return False;
	}

	if(xev->xclient.message_type == motion_event) {
		return !evtype || evtype == SPNAV_EVENT_MOTION ? True : False;
	}
	if(xev->xclient.message_type == button_press_event ||
			xev->xclient.message_type == button_release_event) {
		return !evtype || evtype == SPNAV_EVENT_BUTTON ? True : False;
	}
	return False;
}

static int dropev(int types)
{
	int rm_count = 0;
	XEvent xev;

	if(dpy) return 0;

	while(XCheckIfEvent(dpy, &xev, match_events, (char*)&types)) {
		rm_count++;
	}
	return rm_count;
}

static int set_flt(double val)
{
}

static int get_flt(double *res)
{
}


static int x11_sensitivity(double sens)
{
	int (*prev_xerr_handler)(Display*, XErrorEvent*);
	XEvent xev;
	Window daemon_win;
	float fsens;
	unsigned int isens;

	if(!(daemon_win = get_daemon_window(dpy))) {
		return -1;
	}

	fsens = sens;
	isens = *(unsigned int*)&fsens;

	prev_xerr_handler = XSetErrorHandler(catch_badwin);

	xev.type = ClientMessage;
	xev.xclient.send_event = False;
	xev.xclient.display = dpy;
	xev.xclient.window = app_win;
	xev.xclient.message_type = command_event;
	xev.xclient.format = 16;
	xev.xclient.data.s[0] = isens & 0xffff;
	xev.xclient.data.s[1] = (isens & 0xffff0000) >> 16;
	xev.xclient.data.s[2] = CMD_APP_SENS;

	XSendEvent(dpy, daemon_win, False, 0, &xev);
	XSync(dpy, False);

	XSetErrorHandler(prev_xerr_handler);
	return 0;
}

static Window get_daemon_window(Display *dpy)
{
	Window win, root_win;
	XTextProperty wname;
	Atom type;
	int fmt;
	unsigned long nitems, bytes_after;
	unsigned char *prop;

	root_win = DefaultRootWindow(dpy);

	XGetWindowProperty(dpy, root_win, command_event, 0, 1, False, AnyPropertyType, &type, &fmt, &nitems, &bytes_after, &prop);
	if(!prop) {
		return 0;
	}

	win = *(Window*)prop;
	XFree(prop);

	if(!XGetWMName(dpy, win, &wname) || strcmp("Magellan Window", (char*)wname.value) != 0) {
		return 0;
	}

	return win;
}

static int catch_badwin(Display *dpy, XErrorEvent *err)
{
	char buf[256];

	if(err->error_code == BadWindow) {
		/* do nothing? */
	} else {
		XGetErrorText(dpy, err->error_code, buf, sizeof buf);
		fprintf(stderr, "Caught unexpected X error: %s\n", buf);
	}
	return 0;
}


#else
/* libspnav built without X11 support */

int spnav_x11_init(void)
{
	return -1;
}

int spnav_x11_window(Window win)
{
	return -1;
}

int spnav_x11_event(const XEvent *xev, spnav_event *event)
{
	return -1;
}

#endif
