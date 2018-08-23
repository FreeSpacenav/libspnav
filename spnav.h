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

/* Open connection to the daemon via AF_UNIX socket.
 * The unix domain socket interface is an alternative to the original magellan
 * protocol, and it is *NOT* compatible with the 3D connexion driver. If you wish
 * to remain compatible, use the X11 protocol (spnav_x11_open, see below).
 * Returns -1 on failure.
 */
int spnav_open(void);

/* Close connection to the daemon. Use it for X11 or AF_UNIX connections.
 * Returns -1 on failure
 */
int spnav_close(void);

/* Retrieves the file descriptor used for communication with the daemon, for
 * use with select() by the application, if so required.
 * If the X11 mode is used, the socket used to communicate with the X server is
 * returned, so the result of this function is always reliable.
 * If AF_UNIX mode is used, the fd of the socket is returned or -1 if
 * no connection is open / failure occured.
 */
int spnav_fd(void);

/* TODO: document */
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
/* Opens a connection to the daemon, using the original magellan X11 protocol.
 * Any application using this protocol should be compatible with the proprietary
 * 3D connexion driver too.
 */
int spnav_x11_open(Display *dpy, Window win);

/* Sets the application window, that is to receive events by the driver.
 *
 * NOTE: Any number of windows can be registered for events, when using the
 * free spnavd daemon. I suspect that the proprietary 3D connexion daemon only
 * sends events to one window at a time, thus this function replaces the window
 * that receives events. If compatibility with 3dxsrv is required, do not
 * assume that you can register multiple windows.
 */
int spnav_x11_window(Window win);

/* Examines an arbitrary X11 event. If it's a spnav event, it returns the event
 * type (SPNAV_EVENT_MOTION or SPNAV_EVENT_BUTTON) and fills in the spnav_event
 * structure passed through "event" accordingly. Otherwise, it returns 0.
 */
int spnav_x11_event(const XEvent *xev, spnav_event *event);
#endif


/* ---- spnav 1.x calls ---- */
#define SPNAV_VERSION_1

/* spnav_get_* queries. Using spnav_get_int on a string query will return the
 * length of the string that would be returned by spnav_get_str. Make sure to
 * pass a buffer of at least length + 1 bytes to spnav_get_str, if you wish to
 * avoid truncation. Returned strings are always zero-terminated, even when
 * truncation is necessary.
 *
 * spnav_get_* functions return 0 on success, or -1 on failure.
 */
enum {
	SPNAV_GET_NUM_BUTTONS,	/* int: number of buttons on device */
	SPNAV_GET_DEV_NAME,		/* str: device name */
	SPNAV_GET_HAVE_DISP,	/* int: non-zero if the device has a display */
	SPNAV_GET_DISP_XRES,	/* int: display horizontal resolution */
	SPNAV_GET_DISP_YRES,	/* int: display vertical resolution */
	SPNAV_GET_DISP_COLORS,	/* int: display number of colors */
};
int spnav_get_int(int query, int *res);
int spnav_get_str(int query, char *buf, int bufsz);

/* spnav display functions (for devices which have a display) */

/* after drawing anything, you must call one of the update functions for the
 * changes to be sent to the device and become visible.
 */
/* update the whole display area */
int spnav_disp_update(void);
/* update a specific area of the display */
int spnav_disp_update_rect(int x, int y, int w, int h);

/* map the spacenavd display framebuffer into the client address space for
 * direct drawing. Don't forget to call an update function (see above), in
 * order for any changes to take effect.
 */
void *spnav_disp_map(void);
/* unmap the spacenavd framebuffer shared memory */
int spnav_disp_unmap(void);

/* set color foreground/background to be used by subsequent drawing commands */
int spnav_disp_color(unsigned int fgcol, unsigned int bgcol);

/* position the text cursor for subsequent spnav_disp_print calls */
int spnav_disp_cursor(int row, int col);
/* print a zero-terminated string at the current cursor location. Cursor
 * coordinates will be updated automatically.
 */
int spnav_disp_print(const char *str);

/* clear the whole framebuffer to the current background color */
int spnav_disp_clear(void);
/* draw the outline of a rectangle in the current foreground color */
int spnav_disp_rect(int x, int y, int w, int h);
/* draw a filled rectangle in the current foreground color */
int spnav_disp_fill_rect(int x, int y, int w, int h);
/* draw a line in the current foreground color */
int spnav_disp_line(int x0, int y0, int x1, int y1);


#ifdef __cplusplus
}
#endif

#endif	/* SPACENAV_H_ */
