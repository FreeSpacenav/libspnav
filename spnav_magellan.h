#ifndef SPACENAV_MAGELLAN_H_
#define SPACENAV_MAGELLAN_H_

#include <X11/Xlib.h>

enum {
	MagellanInputMotionEvent = 1,
	MagellanInputButtonEvent = 2,
	MagellanInputButtonReleaseEvent = 3
};

enum {
	MagellanX,
	MagellanY,
	MagellanZ,
	MagellanA,
	MagellanB,
	MagellanC
};

typedef union {
	int data[7];
	int button;
} MagellanIntUnion;

typedef struct {
	int type;
	MagellanIntUnion u;
} MagellanIntEvent;

typedef struct {
	int MagellanType;
	int MagellanButton;
	double MagellanData[6];
	int MagellanPeriod;
} MagellanFloatEvent;


#ifdef __cplusplus
extern "C" {
#endif

int MagellanInit(Display *dpy, Window win);
int MagellanClose(Display *dpy);

int MagellanSetWindow(Display *dpy, Window win);
int MagellanApplicationSensitivity(Display *dpy, double sens);

int MagellanInputEvent(Display *dpy, XEvent *event, MagellanIntEvent *mag_event);
int MagellanTranslateEvent(Display *dpy, XEvent *event, MagellanFloatEvent *mag_event, double tscale, double rscale);
int MagellanRemoveMotionEvents(Display *dpy);

int MagellanRotationMatrix(double mat[4][4], double x, double y, double z);
int MagellanMultiplicationMatrix(double mat_a[4][4], double mat_b[4][4], double mat_c[4][4]);

#ifdef __cplusplus
}
#endif

#endif	/* SPACENAV_MAGELLAN_H_ */
