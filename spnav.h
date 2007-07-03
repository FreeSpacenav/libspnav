/*
This file is part of libspnav, part of the spacenav project (spacenav.sf.net)
Copyright (C) 2007 John Tsiombikas <nuclear@siggraph.org>

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
#ifndef SPACENAV_H_
#define SPACENAV_H_

#include <spnav_config.h>

#ifdef USE_X11
#include <X11/Xlib.h>
#endif

enum {
	SPNAV_EVENT_ANY,	/* used by spnav_remove_events() */
	SPNAV_EVENT_MOTION,
	SPNAV_EVENT_BUTTON	/* includes both press and release */
};

struct spnav_event_motion {
	int type;
	int x, y, z;
	int rx, ry, rz;
	unsigned int period;
	int *data;
};

struct spnav_event_button {
	int type;
	int press;
	int bnum;
};

typedef union spnav_event {
	int type;
	struct spnav_event_motion motion;
	struct spnav_event_button button;
} spnav_event;


#ifdef __cplusplus
extern "C" {
#endif

int spnav_open(void);
int spnav_close(void);

/* Retrieves the file descriptor used for communication with the daemon, for
 * use with select() by the application, if so required.
 * If the X11 mode is used, the socket used to communicate with the X server is
 * returned, so the result of this function is always reliable.
 */
int spnav_fd(void);

int spnav_sensitivity(double sens);

/* blocks waiting for space-nav events. returns 0 if an error occurs */
int spnav_wait_event(spnav_event *event);

/* checks the availability of space-nav events (non-blocking)
 * returns the event type if available, or 0 otherwise.
 */
int spnav_poll_event(spnav_event *event);

/* Removes any pending events from the specified type, or all pending events
 * events if the type argument is SPNAV_EVENT_ANY. Returns the number of
 * removed events.
 */
int spnav_remove_events(int type);




#ifdef USE_X11
int spnav_x11_open(Display *dpy, Window win);
int spnav_x11_window(Window win);

int spnav_x11_event(const XEvent *xev, spnav_event *event);
#endif

#ifdef __cplusplus
}
#endif

#endif	/* SPACENAV_H_ */
