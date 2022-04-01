/*
This file is part of libspnav, part of the spacenav project (spacenav.sf.net)
Copyright (C) 2007-2022 John Tsiombikas <nuclear@member.fsf.org>

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
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include "spnav.h"
#include "proto.h"

/* default timeout for request responses*/
#define TIMEOUT	400
/* default socket path */
#define SPNAV_SOCK_PATH "/var/run/spnav.sock"

#ifdef SPNAV_USE_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static Window get_daemon_window(Display *dpy);
static int catch_badwin(Display *dpy, XErrorEvent *err);

static int read_event(int s, spnav_event *event);
static int proc_event(int *data, spnav_event *event);

static void flush_resp(void);
static int wait_resp(void *buf, int sz, int timeout_ms);
static int request(int req, struct reqresp *rr, int timeout_ms);
static int request_str(int req, char *buf, int bufsz, int timeout_ms);


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
static int proto;

static int connect_afunix(int s, const char *path)
{
	struct sockaddr_un addr = {0};

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof addr.sun_path - 1);

	return connect(s, (struct sockaddr*)&addr, sizeof addr);
}

int spnav_open(void)
{
	int s, cmd;
	char *path;
	FILE *fp;
	char buf[256], *ptr;

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

	/* heed SPNAV_SOCKET environment variable if it's defined */
	if((path = getenv("SPNAV_SOCKET"))) {
		if(connect_afunix(s, path) == 0) goto success;
	}

	/* hacky config file parser, to look for socket = <path> in /etc/spnavrc */
	if((fp = fopen("/etc/spnavrc", "rb"))) {
		path = 0;
		while(fgets(buf, sizeof buf, fp)) {
			ptr = buf;
			while(*ptr && isspace(*ptr)) ptr++;
			if(!*ptr || *ptr == '#') continue;	/* comment or empty line */

			if(memcmp(ptr, "socket", 6) == 0 && (ptr = strchr(ptr, '='))) {
				while(*++ptr && isspace(*ptr));
				if(!*ptr) continue;
				path = ptr;
				ptr += strlen(ptr) - 1;
				while(ptr > path && isspace(*ptr)) *ptr-- = 0;
				break;
			}
		}
		if(path && connect_afunix(s, path) == 0) goto success;
	}

	/* by default use SPNAV_SOCK_PATH (see top of this file) */
	if(connect_afunix(s, SPNAV_SOCK_PATH) == -1) {
		close(s);
		return -1;
	}

success:
	sock = s;
	proto = 0;

	/* send protocol change request and wait for a response.
	 * if we time out, assume we're talking with an old version of spacenavd,
	 * which took our packet as a sensitivity value, so restore sensitivity to
	 * 1.0 and continue with protocol v0
	 */
	cmd = REQ_TAG | REQ_CHANGE_PROTO | MAX_PROTO_VER;
	write(s, &cmd, sizeof cmd);
	if(wait_resp(&cmd, sizeof cmd, 300) == -1) {
		spnav_sensitivity(1.0f);
	} else {
		proto = cmd & 0xff;
	}
	return 0;
}

#ifdef SPNAV_USE_X11
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
		sock = -1;
		return 0;
	}

#ifdef SPNAV_USE_X11
	if(dpy) {
		spnav_x11_window(DefaultRootWindow(dpy));
		app_win = 0;
		dpy = 0;
		return 0;
	}
#endif

	return -1;
}


#ifdef SPNAV_USE_X11
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
	struct reqresp rr;

#ifdef SPNAV_USE_X11
	if(dpy) {
		return x11_sensitivity(sens);
	}
#endif

	if(proto == 0) {
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

	rr.data[0] = *(int*)&sens;
	if(request(REQ_SET_SENS, &rr, TIMEOUT) == -1) {
		return -1;
	}
	return 0;
}

int spnav_fd(void)
{
#ifdef SPNAV_USE_X11
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
	int rd;
	int32_t data[8];

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

	return proc_event(data, event);
}

static int proc_event(int32_t *data, spnav_event *event)
{
	int i;

	if(data[0] < 0 || data[0] >= MAX_UEV) {
		return 0;
	}

	switch(data[0]) {
	case UEV_MOTION:
		event->type = SPNAV_EVENT_MOTION;
		break;
	case UEV_PRESS:
	case UEV_RELEASE:
		event->type = SPNAV_EVENT_BUTTON;
		break;
	default:
		event->type = data[0];
	}

	switch(event->type) {
	case SPNAV_EVENT_MOTION:
		event->motion.data = &event->motion.x;
		for(i=0; i<6; i++) {
			event->motion.data[i] = data[i + 1];
		}
		event->motion.period = data[7];
		break;

	case SPNAV_EVENT_RAWAXIS:
		event->axis.idx = data[1];
		event->axis.value = data[2];
		break;

	case SPNAV_EVENT_BUTTON:
		event->button.press = data[0] == UEV_PRESS ? 1 : 0;
		event->button.bnum = data[1];
		break;

	case SPNAV_EVENT_RAWBUTTON:
		event->button.bnum = data[1];
		event->button.press = data[2];
		break;

	case SPNAV_EVENT_DEV:
		event->dev.op = data[1];
		event->dev.id = data[2];
		event->dev.devtype = data[3];
		event->dev.usbid[0] = data[4];
		event->dev.usbid[1] = data[5];
		break;

	case SPNAV_EVENT_CFG:
		event->cfg.cfg = data[1];
		memcpy(event->cfg.data, data + 2, sizeof event->cfg.data);
		break;
	}

	return event->type;
}


int spnav_wait_event(spnav_event *event)
{
#ifdef SPNAV_USE_X11
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
#ifdef SPNAV_USE_X11
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

#ifdef SPNAV_USE_X11
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

#ifdef SPNAV_USE_X11
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

#ifdef SPNAV_USE_X11
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
		fprintf(stderr, "libspnav: caught unexpected X error: %s\n", buf);
	}
	return 0;
}
#endif

static void flush_resp(void)
{
	int res;
	char buf[256];
	fd_set rdset;
	struct timeval tv = {0};

	FD_ZERO(&rdset);
	FD_SET(sock, &rdset);

	while((res = select(sock + 1, &rdset, 0, 0, &tv)) > 0 || (res == -1 && errno == EINTR)) {
		read(sock, buf, sizeof buf);
	}
}

static int wait_resp(void *buf, int sz, int timeout_ms)
{
	int res;
	fd_set rdset;
	struct timeval tv;
	char *ptr;

	if(timeout_ms) {
		FD_ZERO(&rdset);
		FD_SET(sock, &rdset);

		if(timeout_ms > 0) {
			tv.tv_sec = timeout_ms / 1000;
			tv.tv_usec = (timeout_ms % 1000) * 1000;
		}

		while((res = select(sock + 1, &rdset, 0, 0, timeout_ms < 0 ? 0 : &tv)) == -1 && errno == EINTR);
	}

	if(!timeout_ms || (res > 0 && FD_ISSET(sock, &rdset))) {
		ptr = buf;
		while(sz > 0) {
			if((res = read(sock, ptr, sz)) <= 0 && errno != EINTR) {
				return -1;
			}
			ptr += res;
			sz -= res;
		}
		return 0;
	}
	return -1;
}

static int request(int req, struct reqresp *rr, int timeout_ms)
{
	if(sock < 0 || proto < 1) return -1;

	flush_resp();

	req |= REQ_TAG;
	rr->type = req;

	write(sock, rr, sizeof *rr);
	if(wait_resp(rr, sizeof *rr, TIMEOUT) == -1) {
		return -1;
	}

	/* XXX assuming data[6] is always status */
	if(rr->type != req || rr->data[6] < 0) return -1;
	return 0;
}

static int request_str(int req, char *buf, int bufsz, int timeout_ms)
{
	int res = -1;
	struct reqresp rr = {0};
	struct reqresp_strbuf sbuf = {0};

	if(request(req, &rr, timeout_ms) == -1) {
		return -1;
	}

	while((res = spnav_recv_str(&sbuf, &rr)) == 0) {
		if(wait_resp(&rr, sizeof rr, timeout_ms) == -1) {
			free(sbuf.buf);
			return -1;
		}
	}

	if(res == -1) {
		free(sbuf.buf);
		return -1;
	}

	if(buf) {
		strncpy(buf, sbuf.buf, bufsz - 1);
		buf[bufsz - 1] = 0;
	}
	free(sbuf.buf);
	return sbuf.size - 1;
}



int spnav_protocol(void)
{
	return proto;
}

int spnav_client_name(const char *name)
{
	return spnav_send_str(sock, REQ_SET_NAME, name);
}

int spnav_evmask(unsigned int mask)
{
	struct reqresp rr = {0};
	rr.data[0] = mask;
	if(request(REQ_SET_EVMASK, &rr, TIMEOUT) == -1) {
		return -1;
	}
	return 0;
}

int spnav_dev_name(char *buf, int bufsz)
{
	return request_str(REQ_DEV_NAME, buf, bufsz, TIMEOUT);
}

int spnav_dev_path(char *buf, int bufsz)
{
	return request_str(REQ_DEV_PATH, buf, bufsz, TIMEOUT);
}

int spnav_dev_buttons(void)
{
	struct reqresp rr = {0};
	if(request(REQ_DEV_NBUTTONS, &rr, TIMEOUT) == -1) {
		return 2;	/* default */
	}
	return rr.data[0];
}

int spnav_dev_axes(void)
{
	struct reqresp rr = {0};
	if(request(REQ_DEV_NAXES, &rr, TIMEOUT) == -1) {
		return 6;	/* default */
	}
	return rr.data[0];
}

int spnav_dev_usbid(unsigned int *vend, unsigned int *prod)
{
	struct reqresp rr = {0};
	if(request(REQ_DEV_USBID, &rr, TIMEOUT) == -1) {
		return -1;
	}
	if(vend) *vend = rr.data[0];
	if(prod) *prod = rr.data[1];
	return 0;
}

int spnav_dev_type(void)
{
	struct reqresp rr = {0};
	if(request(REQ_DEV_TYPE, &rr, TIMEOUT) == -1) {
		return -1;
	}
	return rr.data[0];
}


/* configuation api */

int spnav_cfg_reset(void)
{
	struct reqresp rr = {0};
	return request(REQ_CFG_RESET, &rr, TIMEOUT);
}

int spnav_cfg_restore(void)
{
	struct reqresp rr = {0};
	return request(REQ_CFG_RESTORE, &rr, TIMEOUT);
}

int spnav_cfg_save(void)
{
	struct reqresp rr = {0};
	return request(REQ_CFG_SAVE, &rr, TIMEOUT);
}

int spnav_cfg_set_sens(float s)
{
	struct reqresp rr = {0};
	rr.data[0] = *(int*)&s;
	return request(REQ_SCFG_SENS, &rr, TIMEOUT);
}

float spnav_cfg_get_sens(void)
{
	struct reqresp rr = {0};
	if(request(REQ_GCFG_SENS, &rr, TIMEOUT) == -1) {
		return -1.0f;
	}
	return *(float*)&rr.data[0];
}

int spnav_cfg_set_axis_sens(const float *svec)
{
	struct reqresp rr;
	memcpy(rr.data, svec, 6 * sizeof *svec);
	return request(REQ_SCFG_SENS_AXIS, &rr, TIMEOUT);
}

int spnav_cfg_get_axis_sens(float *svec)
{
	struct reqresp rr = {0};

	if(request(REQ_GCFG_SENS_AXIS, &rr, TIMEOUT) == -1) {
		return -1;
	}
	memcpy(svec, rr.data, 6 * sizeof *svec);
	return 0;
}

int spnav_cfg_set_deadzone(int axis, int delta)
{
	struct reqresp rr = {0};

	rr.data[0] = axis;
	rr.data[1] = delta;
	return request(REQ_SCFG_DEADZONE, &rr, TIMEOUT);
}

int spnav_cfg_get_deadzone(int axis)
{
	struct reqresp rr = {0};

	rr.data[0] = axis;
	if(request(REQ_GCFG_DEADZONE, &rr, TIMEOUT) == -1) {
		return -1;
	}
	return rr.data[1];
}

int spnav_cfg_set_invert(int invbits)
{
	int i;
	struct reqresp rr;

	for(i=0; i<6; i++) {
		rr.data[i] = invbits & 1;
		invbits >>= 1;
	}
	return request(REQ_SCFG_INVERT, &rr, TIMEOUT);
}

int spnav_cfg_get_invert(void)
{
	int i, res = 0;
	struct reqresp rr = {0};

	if(request(REQ_GCFG_INVERT, &rr, TIMEOUT) == -1) {
		return -1;
	}
	for(i=0; i<6; i++) {
		res = (res >> 1) | (rr.data[i] ? 0x20 : 0);
	}
	return res;
}

int spnav_cfg_set_axismap(int devaxis, int map)
{
	struct reqresp rr = {0};

	rr.data[0] = devaxis;
	rr.data[1] = map;
	return request(REQ_SCFG_AXISMAP, &rr, TIMEOUT);
}

int spnav_cfg_get_axismap(int devaxis)
{
	struct reqresp rr = {0};

	rr.data[0] = devaxis;
	if(request(REQ_GCFG_AXISMAP, &rr, TIMEOUT) == -1) {
		return -1;
	}
	return rr.data[1];
}

int spnav_cfg_set_bnmap(int devbn, int map)
{
	struct reqresp rr = {0};

	rr.data[0] = devbn;
	rr.data[1] = map;
	return request(REQ_SCFG_BNMAP, &rr, TIMEOUT);
}

int spnav_cfg_get_bnmap(int devbn)
{
	struct reqresp rr = {0};

	rr.data[0] = devbn;
	if(request(REQ_GCFG_BNMAP, &rr, TIMEOUT) == -1) {
		return -1;
	}
	return rr.data[1];
}

int spnav_cfg_set_bnaction(int bn, int act)
{
	struct reqresp rr = {0};

	rr.data[0] = bn;
	rr.data[1] = act;
	return request(REQ_SCFG_BNACTION, &rr, TIMEOUT);
}

int spnav_cfg_get_bnaction(int bn)
{
	struct reqresp rr = {0};

	rr.data[0] = bn;
	if(request(REQ_GCFG_BNACTION, &rr, TIMEOUT) == -1) {
		return -1;
	}
	return rr.data[1];
}

int spnav_cfg_set_kbmap(int bn, int key)
{
	struct reqresp rr = {0};

	rr.data[0] = bn;
	rr.data[1] = key;
	return request(REQ_SCFG_KBMAP, &rr, TIMEOUT);
}

int spnav_cfg_get_kbmap(int bn)
{
	struct reqresp rr = {0};

	rr.data[0] = bn;
	if(request(REQ_GCFG_KBMAP, &rr, TIMEOUT) == -1) {
		return -1;
	}
	return rr.data[1];
}

int spnav_cfg_set_swapyz(int swap)
{
	struct reqresp rr = {0};

	rr.data[0] = swap;
	return request(REQ_SCFG_SWAPYZ, &rr, TIMEOUT);
}

int spnav_cfg_get_swapyz(void)
{
	struct reqresp rr = {0};

	if(request(REQ_GCFG_SWAPYZ, &rr, TIMEOUT) == -1) {
		return -1;
	}
	return rr.data[0];
}

int spnav_cfg_set_led(int state)
{
	struct reqresp rr = {0};

	if(state < 0 || state >= 3) return -1;

	rr.data[0] = state;
	return request(REQ_SCFG_LED, &rr, TIMEOUT);
}

int spnav_cfg_get_led(void)
{
	struct reqresp rr = {0};

	if(request(REQ_GCFG_LED, &rr, TIMEOUT) == -1) {
		return -1;
	}
	return rr.data[0];
}

int spnav_cfg_set_grab(int state)
{
	struct reqresp rr = {0};

	rr.data[0] = state ? 1 : 0;
	return request(REQ_SCFG_GRAB, &rr, TIMEOUT);
}

int spnav_cfg_get_grab(void)
{
	struct reqresp rr = {0};

	if(request(REQ_GCFG_GRAB, &rr, TIMEOUT) == -1) {
		return -1;
	}
	return rr.data[0];
}

int spnav_cfg_set_serial(const char *devpath)
{
	return spnav_send_str(sock, REQ_SCFG_SERDEV, devpath);
}

int spnav_cfg_get_serial(char *buf, int bufsz)
{
	return request_str(REQ_GCFG_SERDEV, buf, bufsz, TIMEOUT);
}
