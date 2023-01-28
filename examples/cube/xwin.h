#ifndef XWIN_H_
#define XWIN_H_

#include <X11/Xlib.h>

extern Display *dpy;
extern Window win;
extern int win_width, win_height;
extern int redisplay_pending;

int create_xwin(const char *title, int xsz, int ysz);
void destroy_xwin(void);

#endif	/* XWIN_H_ */
