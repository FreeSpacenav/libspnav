/* This example demonstrates how to use 6-dof input for controlling the camera
 * to fly around a scene. The native spacenav protocol is used, to also
 * demonstrate how to integrate input from libspnav in an event loop.
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

int create_gfx(int xsz, int ysz);
void destroy_gfx(void);
void set_window_title(const char *title);
void gen_textures(void);
void redraw(void);
void draw_scene(void);
void draw_box(float xsz, float ysz, float zsz);
int handle_xevent(XEvent *xev);
void handle_spnav_event(spnav_event *ev);


Display *dpy;
Atom wm_prot, wm_del_win;
GLXContext ctx;
Window win;

struct spnav_posrot posrot;

int redisplay;

unsigned int grid_tex, box_tex;


int main(void)
{
	int xsock, ssock, maxfd;

	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "failed to connect to the X server");
		return 1;
	}

	if(create_gfx(1024, 768) == -1) {
		return 1;
	}

	/* XXX: open connection to the spacenav driver */
	if(spnav_open() == -1) {
		fprintf(stderr, "failed to connect to the spacenav driver\n");
		return 1;
	}

	spnav_posrot_init(&posrot);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	gen_textures();

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

		if(redisplay) {
			redraw();
			redisplay = 0;
		}
	}

end:
	destroy_gfx();
	spnav_close();
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
	glDeleteTextures(1, &grid_tex);

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


void gen_textures(void)
{
	int i, j;
	static unsigned char grid_pixels[128 * 128 * 3];
	unsigned char *pptr;

	pptr = grid_pixels;
	for(i=0; i<128; i++) {
		float dy = abs(i - 64) / 64.0f;
		for(j=0; j<128; j++) {
			float dx = abs(j - 64) / 64.0f;
			float d = dx > dy ? dx : dy;
			float val = pow(d, 8.0);
			if(val > 1.0f) val = 1.0f;

			*pptr++ = (int)(214.0f * val);
			*pptr++ = (int)(76.0f * val);
			*pptr++ = (int)(255.0f * val);
		}
	}

	glGenTextures(1, &grid_tex);
	glBindTexture(GL_TEXTURE_2D, grid_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, 128, 128, GL_RGB, GL_UNSIGNED_BYTE, grid_pixels);
}

#define GRID_REP	60
#define GRID_SZ		200

void redraw(void)
{
	float xform[16];

	/* XXX convert the accumulated position/rotation into a 4x4 view matrix */
	spnav_matrix_view(xform, &posrot);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf(xform);

	glBindTexture(GL_TEXTURE_2D, grid_tex);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(-GRID_SZ, 0, GRID_SZ);
	glTexCoord2f(GRID_REP, 0);
	glVertex3f(GRID_SZ, 0, GRID_SZ);
	glTexCoord2f(GRID_REP, GRID_REP);
	glVertex3f(GRID_SZ, 0, -GRID_SZ);
	glTexCoord2f(0, GRID_REP);
	glVertex3f(-GRID_SZ, 0, -GRID_SZ);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	draw_box(1, 1, 1);

	glXSwapBuffers(dpy, win);
}

void draw_scene(void)
{
}

void draw_box(float xsz, float ysz, float zsz)
{
	xsz /= 2;
	ysz /= 2;
	zsz /= 2;

	glBegin(GL_QUADS);
	/* face +Z */
	glNormal3f(0, 0, 1);
	glVertex3f(-xsz, -ysz, zsz);
	glVertex3f(xsz, -ysz, zsz);
	glVertex3f(xsz, ysz, zsz);
	glVertex3f(-xsz, ysz, zsz);
	/* face +X */
	glNormal3f(1, 0, 0);
	glVertex3f(xsz, -ysz, zsz);
	glVertex3f(xsz, -ysz, -zsz);
	glVertex3f(xsz, ysz, -zsz);
	glVertex3f(xsz, ysz, zsz);
	/* face -Z */
	glNormal3f(0, 0, -1);
	glVertex3f(xsz, -ysz, -zsz);
	glVertex3f(-xsz, -ysz, -zsz);
	glVertex3f(-xsz, ysz, -zsz);
	glVertex3f(xsz, ysz, -zsz);
	/* face -X */
	glNormal3f(-1, 0, 0);
	glVertex3f(-xsz, -ysz, -zsz);
	glVertex3f(-xsz, -ysz, zsz);
	glVertex3f(-xsz, ysz, zsz);
	glVertex3f(-xsz, ysz, -zsz);
	/* face +Y */
	glNormal3f(0, 1, 0);
	glVertex3f(-xsz, ysz, zsz);
	glVertex3f(xsz, ysz, zsz);
	glVertex3f(xsz, ysz, -zsz);
	glVertex3f(-xsz, ysz, -zsz);
	/* face -Y */
	glNormal3f(0, -1, 0);
	glVertex3f(-xsz, -ysz, -zsz);
	glVertex3f(xsz, -ysz, -zsz);
	glVertex3f(xsz, -ysz, zsz);
	glVertex3f(-xsz, -ysz, zsz);
	glEnd();
}

int handle_xevent(XEvent *xev)
{
	int x, y;
	KeySym sym;

	switch(xev->type) {
	case Expose:
		redisplay = 1;
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

		glViewport(0, 0, x, y);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(50.0, (float)x / (float)y, 0.5, 500.0);
		break;

	default:
		break;
	}
	return 0;
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
		redisplay = 1;
		break;

	case SPNAV_EVENT_BUTTON:
		/* XXX reset position and orientation to identity on button presses to
		 * reset the view
		 */
		spnav_posrot_init(&posrot);
		redisplay = 1;
		break;

	default:
		break;
	}
}
