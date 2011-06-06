/*
This file is part of libspnav, part of the spacenav project (spacenav.sf.net)
Copyright (C) 2007-2010 John Tsiombikas <nuclear@member.fsf.org>

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include "spnav.h"

#define SPNAV_SOCK_PATH "/var/run/spnav.sock"

#ifdef USE_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static Window get_daemon_window(Display *dpy);
static int catch_badwin(Display *dpy, XErrorEvent *err);

static Display *dpy;
static Window app_win;
static Atom motion_event, button_press_event, button_release_event, command_event;

enum {
	CMD_APP_WINDOW = 27695,
	CMD_APP_SENS
};

#define IS_OPEN		(dpy || (sock != -1))
#else
#define IS_OPEN		(sock != -1)
#endif

struct event_node {
	spnav_event event;
	struct event_node *next;
};

/* only used for non-X mode, with spnav_remove_events */
static struct event_node *ev_queue, *ev_queue_tail;

/* AF_UNIX socket used for alternative communication with daemon */
static int sock = -1;


int spnav_open(void)
{
	int s;
	struct sockaddr_un addr;

	if(IS_OPEN) {
		return -1;
	}

	if(!(ev_queue = malloc(sizeof *ev_queue))) {
		return -1;
	}
	ev_queue->next = 0;
	ev_queue_tail = ev_queue;

	if((s = socket(PF_UNIX, SOCK_STREAM, 0)) == -1) {
		return -1;
	}

	memset(&addr, 0, sizeof addr);
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SPNAV_SOCK_PATH, sizeof(addr.sun_path));


	if(connect(s, (struct sockaddr*)&addr, sizeof addr) == -1) {
		perror("connect failed");
		return -1;
	}

	sock = s;
	return 0;
}

#ifdef USE_X11
int spnav_x11_open(Display *display, Window win)
{
	if(IS_OPEN) {
		return -1;
	}

	dpy = display;

	motion_event = XInternAtom(dpy, "MotionEvent", True);
	button_press_event = XInternAtom(dpy, "ButtonPressEvent", True);
	button_release_event = XInternAtom(dpy, "ButtonReleaseEvent", True);
	command_event = XInternAtom(dpy, "CommandEvent", True);

	if(!motion_event || !button_press_event || !button_release_event || !command_event) {
		dpy = 0;
		return -1;	/* daemon not started */
	}

	if(spnav_x11_window(win) == -1) {
		dpy = 0;
		return -1;	/* daemon not started */
	}

	app_win = win;
	return 0;
}
#endif

int spnav_close(void)
{
	if(!IS_OPEN) {
		return -1;
	}

	if(sock) {
		while(ev_queue) {
			void *tmp = ev_queue;
			ev_queue = ev_queue->next;
			free(tmp);
		}

		close(sock);
		sock = 0;
		return 0;
	}

#ifdef USE_X11
	if(dpy) {
		spnav_x11_window(DefaultRootWindow(dpy));
		app_win = 0;
		dpy = 0;
		return 0;
	}
#endif

	return -1;
}


#ifdef USE_X11
int spnav_x11_window(Window win)
{
	int (*prev_xerr_handler)(Display*, XErrorEvent*);
	XEvent xev;
	Window daemon_win;

	if(!IS_OPEN) {
		return -1;
	}

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
#endif

int spnav_sensitivity(double sens)
{
#ifdef USE_X11
	if(dpy) {
		return x11_sensitivity(sens);
	}
#endif

	if(sock) {
		ssize_t bytes;
		float fval = sens;

		while((bytes = write(sock, &fval, sizeof fval)) <= 0 && errno == EINTR);
		if(bytes <= 0) {
			return -1;
		}
		return 0;
	}

	return -1;
}

int spnav_fd(void)
{
#ifdef USE_X11
	if(dpy) {
		return ConnectionNumber(dpy);
	}
#endif

	return sock;
}


/* Checks both the event queue and the daemon socket for pending events.
 * In either case, it returns immediately with true/false values (doesn't block).
 */
static int event_pending(int s)
{
	fd_set rd_set;
	struct timeval tv;

	if(ev_queue->next) {
		return 1;
	}

	FD_ZERO(&rd_set);
	FD_SET(s, &rd_set);

	/* don't block, just poll */
	tv.tv_sec = tv.tv_usec = 0;

	if(select(s + 1, &rd_set, 0, 0, &tv) > 0) {
		return 1;
	}
	return 0;
}

/* If there are events waiting in the event queue, dequeue one and
 * return that, otherwise read one from the daemon socket.
 * This might block unless we called event_pending() first and it returned true.
 */
static int read_event(int s, spnav_event *event)
{
	int i, rd;
	int data[8];

	/* if we have a queued event, deliver that one */
	if(ev_queue->next) {
		struct event_node *node = ev_queue->next;
		ev_queue->next = ev_queue->next->next;

		/* dequeued the last event, must update tail pointer */
		if(ev_queue_tail == node) {
			ev_queue_tail = ev_queue;
		}

		memcpy(event, &node->event, sizeof *event);
		free(node);
		return event->type;
	}

	/* otherwise read one from the connection */
	do {
		rd = read(s, data, sizeof data);
	} while(rd == -1 && errno == EINTR);

	if(rd <= 0) {
		return 0;
	}

	if(data[0] < 0 || data[0] > 2) {
		return 0;
	}
	event->type = data[0] ? SPNAV_EVENT_BUTTON : SPNAV_EVENT_MOTION;

	if(event->type == SPNAV_EVENT_MOTION) {
		event->motion.data = &event->motion.x;
		for(i=0; i<6; i++) {
			event->motion.data[i] = data[i + 1];
		}
		event->motion.period = data[7];
	} else {
		event->button.press = data[0] == 1 ? 1 : 0;
		event->button.bnum = data[1];
	}

	return event->type;
}


int spnav_wait_event(spnav_event *event)
{
#ifdef USE_X11
	if(dpy) {
		for(;;) {
			XEvent xev;
			XNextEvent(dpy, &xev);

			if(spnav_x11_event(&xev, event) > 0) {
				return event->type;
			}
		}
	}
#endif

	if(sock) {
		if(read_event(sock, event) > 0) {
			return event->type;
		}
	}
	return 0;
}

int spnav_poll_event(spnav_event *event)
{
#ifdef USE_X11
	if(dpy) {
		if(XPending(dpy)) {
			XEvent xev;
			XNextEvent(dpy, &xev);

			return spnav_x11_event(&xev, event);
		}
		return 0;
	}
#endif

	if(sock) {
		if(event_pending(sock)) {
			if(read_event(sock, event) > 0) {
				return event->type;
			}
		}
	}
	return 0;
}

#ifdef USE_X11
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
#endif

/* Appends an event to an event list.
 * Tailptr must be a pointer to the tail pointer of the list. NULL means
 * append to the global event queue.
 */
static int enqueue_event(spnav_event *event, struct event_node **tailptr)
{
	struct event_node *node;
	if(!(node = malloc(sizeof *node))) {
		return -1;
	}

	node->event = *event;
	node->next = 0;

	if(!tailptr) {
		tailptr = &ev_queue_tail;
	}

	(*tailptr)->next = node;
	*tailptr = node;
	return 0;
}

int spnav_remove_events(int type)
{
	int rm_count = 0;

#ifdef USE_X11
	if(dpy) {
		XEvent xev;

		while(XCheckIfEvent(dpy, &xev, match_events, (char*)&type)) {
			rm_count++;
		}
		return rm_count;
	}
#endif

	if(sock) {
		struct event_node *tmplist, *tmptail;

		if(!(tmplist = tmptail = malloc(sizeof *tmplist))) {
			return -1;
		}
		tmplist->next = 0;

		/* while there are events in the event queue, or the daemon socket */
		while(event_pending(sock)) {
			spnav_event event;

			read_event(sock, &event);	/* remove next event */
			if(event.type != type) {
				/* We don't want to drop this one, wrong type. Keep the event
				 * in the temporary list, for deferred reinsertion
				 */
				enqueue_event(&event, &tmptail);
			} else {
				rm_count++;
			}
		}

		/* reinsert any events we removed that we didn't mean to */
		while(tmplist->next) {
			struct event_node *node = tmplist->next;

			enqueue_event(&node->event, 0);

			free(tmplist);
			tmplist = node;
		}
		free(tmplist);

		return rm_count;
	}
	return 0;
}

#ifdef USE_X11
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

int catch_badwin(Display *dpy, XErrorEvent *err)
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
#endif
