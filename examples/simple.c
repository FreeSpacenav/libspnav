#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <spnav.h>

void sig(int s)
{
	spnav_close();
	exit(0);
}

int main(void)
{

#if defined(BUILD_X11)
	Display *dpy;
	Window win;
	unsigned long bpix;
#endif

	spnav_event sev;

	signal(SIGINT, sig);

#if defined(BUILD_X11)

	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "failed to connect to the X server\n");
		return 1;
	}
	bpix = BlackPixel(dpy, DefaultScreen(dpy));
	win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, bpix, bpix);

	/* This actually registers our window with the driver for receiving
	 * motion/button events through the 3dxsrv-compatible X11 protocol.
	 */
	if(spnav_x11_open(dpy, win) == -1) {
		fprintf(stderr, "failed to connect to the space navigator daemon\n");
		return 1;
	}

#elif defined(BUILD_AF_UNIX)
	if(spnav_open()==-1) {
	  	fprintf(stderr, "failed to connect to the space navigator daemon\n");
		return 1;
	}
#else
#error Unknown build type!
#endif

	/* spnav_wait_event() and spnav_poll_event(), will silently ignore any non-spnav X11 events.
	 *
	 * If you need to handle other X11 events you will have to use a regular XNextEvent() loop,
	 * and pass any ClientMessage events to spnav_x11_event, which will return the event type or
	 * zero if it's not an spnav event (see spnav.h).
	 */
	while(spnav_wait_event(&sev)) {
		if(sev.type == SPNAV_EVENT_MOTION) {
			printf("got motion event: t(%d, %d, %d) ", sev.motion.x, sev.motion.y, sev.motion.z);
			printf("r(%d, %d, %d)\n", sev.motion.rx, sev.motion.ry, sev.motion.rz);
		} else {	/* SPNAV_EVENT_BUTTON */
			printf("got button %s event b(%d)\n", sev.button.press ? "press" : "release", sev.button.bnum);
		}
	}

	spnav_close();
	return 0;
}
