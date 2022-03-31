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
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <spnav.h>

#define SQ(x)	((x) * (x))

int create_gfx(int xsz, int ysz);
void destroy_gfx(void);
void set_window_title(const char *title);
void redraw(void);
void draw_cube(void);
int handle_event(XEvent *xev);


Display *dpy;
Atom wm_prot, wm_del_win;
GLXContext ctx;
Window win;

/* XXX: posrot contains a position vector and an orientation quaternion, and
 * can be used with the spnav_posrot_moveobj function to accumulate input
 * motions, and then with spnav_matrix_obj to create a transformation matrix.
 * See util.c in the libspnav source code for implementation details.
 */
struct spnav_posrot posrot;

int redisplay;


int main(void)
{
	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "failed to connect to the X server");
		return 1;
	}

	if(create_gfx(512, 512) == -1) {
		return 1;
	}

	/* XXX: spnav_x11_open registers our window with the driver for receiving
	 * motion/button events through the 3dxsrv-compatible X11 magellan protocol.
	 */
	if(spnav_x11_open(dpy, win) == -1) {
		fprintf(stderr, "failed to connect to the space navigator daemon\n");
		return 1;
	}
	/* XXX: initialize the position vector & orientation quaternion */
	spnav_posrot_init(&posrot);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	for(;;) {
		XEvent xev;
		XNextEvent(dpy, &xev);

		if(handle_event(&xev) != 0) {
			destroy_gfx();
			XCloseDisplay(dpy);
			return 0;
		}

		if(redisplay) {
			redraw();
			redisplay = 0;
		}
	}
	return 0;
}

int create_gfx(int xsz, int ysz)
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

	set_window_title("libspnav cube");

	class_hint.res_name = "cube";
	class_hint.res_class = "cube";
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

void destroy_gfx(void)
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

void redraw(void)
{
	float xform[16];

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -6);	/* view matrix, push back to see the cube */

	/* XXX convert the accumulated position/rotation into a 4x4 model matrix */
	spnav_matrix_obj(xform, &posrot);
	glMultMatrixf(xform);		/* concatenate our computed model matrix */

	draw_cube();

	glXSwapBuffers(dpy, win);
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

int handle_event(XEvent *xev)
{
	static int win_mapped;
	KeySym sym;
	spnav_event spev;

	switch(xev->type) {
	case MapNotify:
		win_mapped = 1;
		break;

	case UnmapNotify:
		win_mapped = 0;
		break;

	case Expose:
		if(win_mapped && xev->xexpose.count == 0) {
			redraw();
		}
		break;

	case ClientMessage:
		/* XXX check if the event is a spacenav event */
		if(spnav_x11_event(xev, &spev)) {
			/* if so deal with motion and button events */
			if(spev.type == SPNAV_EVENT_MOTION) {
				/* XXX use the spnav_posrot_moveobj utility function to
				 * accumulate motion inputs into the cube's position vector and
				 * orientation quaternion.
				 */
				spnav_posrot_moveobj(&posrot, &spev.motion);

				redisplay = 1;
			} else {
				/* XXX on button press, reset the cube position/orientation */
				if(spev.button.press) {
					spnav_posrot_init(&posrot);

					redisplay = 1;
				}
			}
			/* finally remove any other queued motion events */
			spnav_remove_events(SPNAV_EVENT_MOTION);

		} else if(xev->xclient.message_type == wm_prot) {
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
		{
			int x = xev->xconfigure.width;
			int y = xev->xconfigure.height;

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluPerspective(45.0, (float)x / (float)y, 1.0, 1000.0);

			glViewport(0, 0, x, y);
		}
		break;

	default:
		break;
	}
	return 0;
}
