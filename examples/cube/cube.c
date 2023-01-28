/* This example demonstrates how to use libspnav to get 6dof input,
 * and use that to rotate and translate a 3D cube. The magellan X11 protocol is
 * used (spnav_x11_open) which is compatible with both spacenavd and
 * 3Dconnexion's 3dxsrv.
 *
 * The code is a bit cluttered with X11 and GLX calls, so the interesting bits
 * are marked with XXX comments.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <sys/select.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <spnav.h>
#include "xwin.h"

#define SQ(x)	((x) * (x))

void gen_scene(void);
void redraw(void);
void draw_cube(void);
void handle_spnav_event(spnav_event *ev);
int handle_xevent(XEvent *xev);

/* XXX: posrot contains a position vector and an orientation quaternion, and
 * can be used with the spnav_posrot_moveobj function to accumulate input
 * motions, and then with spnav_matrix_obj to create a transformation matrix.
 * See util.c in the libspnav source code for implementation details.
 */
struct spnav_posrot posrot;

unsigned int scene;


int main(void)
{
	int xsock, ssock, maxfd;

	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "failed to connect to the X server");
		return 1;
	}

	if(create_xwin("libspnav fly", 1024, 768) == -1) {
		return 1;
	}

	/* XXX: open connection to the spacenav driver */
	if(spnav_open() == -1) {
		fprintf(stderr, "failed to connect to the spacenav driver\n");
		return 1;
	}
	/* XXX: initialize the position vector & orientation quaternion */
	spnav_posrot_init(&posrot);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	gen_scene();

	/* XXX: grab the Xlib socket and the libspnav socket. we'll need them in the
	 * select loop to wait for input from either source.
	 */
	xsock = ConnectionNumber(dpy);		/* Xlib socket */
	ssock = spnav_fd();					/* libspnav socket */
	maxfd = xsock > ssock ? xsock : ssock;

	for(;;) {
		fd_set rdset;

		/* XXX: add both sockets to the file descriptor set, to monitor both */
		FD_ZERO(&rdset);
		FD_SET(xsock, &rdset);
		FD_SET(ssock, &rdset);

		while(select(maxfd + 1, &rdset, 0, 0, 0) == -1 && errno == EINTR);

		/* XXX: handle any pending X events */
		if(FD_ISSET(xsock, &rdset)) {
			while(XPending(dpy)) {
				XEvent xev;
				XNextEvent(dpy, &xev);

				if(handle_xevent(&xev) != 0) {
					goto end;
				}
			}
		}

		/* XXX: handle any pending spacenav events */
		if(FD_ISSET(ssock, &rdset)) {
			spnav_event sev;
			while(spnav_poll_event(&sev)) {
				handle_spnav_event(&sev);
			}
		}

		if(redisplay_pending) {
			redisplay_pending = 0;
			redraw();
		}
	}

end:
	destroy_xwin();
	spnav_close();
	return 0;
}

void gen_scene(void)
{
	srand(0);

	scene = glGenLists(1);
	glNewList(scene, GL_COMPILE);

	draw_cube();

	glEndList();
}

void redraw(void)
{
	float xform[16];

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(-2, 0, 0);		/* move the default view a bit higher above the ground */

	/* XXX convert the accumulated position/rotation into a 4x4 view matrix */
	spnav_matrix_view(xform, &posrot);
	glMultMatrixf(xform);		/* concatenate our computed view matrix */

	draw_cube();

	glXSwapBuffers(dpy, win);
}

void reshape(int x, int y)
{
	glViewport(0, 0, x, y);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50.0, (float)x / (float)y, 0.5, 500.0);
}

void draw_cube(void)
{
	glBegin(GL_QUADS);
	/* face +Z */
	glNormal3f(0, 0, 1);
	glColor3f(1, 0, 0);
	glVertex3f(-1, -1, 1);
	glVertex3f(1, -1, 1);
	glVertex3f(1, 1, 1);
	glVertex3f(-1, 1, 1);
	/* face +X */
	glNormal3f(1, 0, 0);
	glColor3f(0, 1, 0);
	glVertex3f(1, -1, 1);
	glVertex3f(1, -1, -1);
	glVertex3f(1, 1, -1);
	glVertex3f(1, 1, 1);
	/* face -Z */
	glNormal3f(0, 0, -1);
	glColor3f(0, 0, 1);
	glVertex3f(1, -1, -1);
	glVertex3f(-1, -1, -1);
	glVertex3f(-1, 1, -1);
	glVertex3f(1, 1, -1);
	/* face -X */
	glNormal3f(-1, 0, 0);
	glColor3f(1, 1, 0);
	glVertex3f(-1, -1, -1);
	glVertex3f(-1, -1, 1);
	glVertex3f(-1, 1, 1);
	glVertex3f(-1, 1, -1);
	/* face +Y */
	glNormal3f(0, 1, 0);
	glColor3f(0, 1, 1);
	glVertex3f(-1, 1, 1);
	glVertex3f(1, 1, 1);
	glVertex3f(1, 1, -1);
	glVertex3f(-1, 1, -1);
	/* face -Y */
	glNormal3f(0, -1, 0);
	glColor3f(1, 0, 1);
	glVertex3f(-1, -1, -1);
	glVertex3f(1, -1, -1);
	glVertex3f(1, -1, 1);
	glVertex3f(-1, -1, 1);
	glEnd();
}

void handle_spnav_event(spnav_event *ev)
{
	switch(ev->type) {
	case SPNAV_EVENT_MOTION:
		/* XXX use the spnav_posrot_moveview utility function to modify the view
		 * position vector and orientation quaternion, based on the motion input
		 * we received.
		 *
		 * We'll also divide rotation values by two, to make rotation less
		 * sensitive. In this scale, it feels better that way in fly mode. The
		 * ideal ratio of sensitivities will vary depending on the scene scale.
		 */
		ev->motion.rx /= 2;
		ev->motion.ry /= 2;
		ev->motion.rz /= 2;
		spnav_posrot_moveview(&posrot, &ev->motion);

		/* XXX: Drop any further pending motion events. This can make our input
		 * more responsive on slow or heavily loaded machines. We don't gain
		 * anything by processing a whole queue of relative motions.
		 */
		spnav_remove_events(SPNAV_EVENT_MOTION);
		redisplay_pending = 1;
		break;

	case SPNAV_EVENT_BUTTON:
		/* XXX reset position and orientation to identity on button presses to
		 * reset the view
		 */
		spnav_posrot_init(&posrot);
		redisplay_pending = 1;
		break;

	default:
		break;
	}
}
