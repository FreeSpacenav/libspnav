#include "spnav_magellan.h"
#include "spnav.h"

int MagellanInit(Display *dpy, Window win)
{
	return spnav_x11_open(dpy, win) == -1 ? 0 : 1;
}

int MagellanClose(Display *dpy)
{
	return spnav_close() == -1 ? 0 : 1;
}

int MagellanSetWindow(Display *dpy, Window win)
{
	return spnav_x11_window(win) == -1 ? 0 : 1;
}

int MagellanApplicationSensitivity(Display *dpy, double sens)
{
	return spnav_sensitivity(sens) == -1 ? 0 : 1;
}


int MagellanInputEvent(Display *dpy, XEvent *xev, MagellanIntEvent *mev)
{
	int i;
	spnav_event event;

	if(!spnav_x11_event(xev, &event)) {
		return 0;
	}

	if(event.type == SPNAV_EVENT_MOTION) {
		mev->type = MagellanInputMotionEvent;

		for(i=0; i<6; i++) {
			mev->u.data[i] = event.motion.data[i];
		}
		mev->u.data[6] = event.motion.period * 1000 / 60;
	} else {
		mev->type = event.button.press ? MagellanInputButtonEvent : MagellanInputButtonReleaseEvent;
		mev->u.button = event.button.bnum;
	}

	return mev->type;
}


int MagellanTranslateEvent(Display *dpy, XEvent *xev, MagellanFloatEvent *mev, double tscale, double rscale)
{
	int i;
	spnav_event event;

	if(!spnav_x11_event(xev, &event)) {
		return 0;
	}

	if(event.type == SPNAV_EVENT_MOTION) {
		mev->MagellanType = MagellanInputMotionEvent;

		for(i=0; i<6; i++) {
			mev->MagellanData[i] = (double)event.motion.data[i] * (i < 3 ? tscale : rscale);
		}
		mev->MagellanPeriod = event.motion.period;
	} else {
		mev->MagellanType = event.button.press ? MagellanInputButtonEvent : MagellanInputButtonReleaseEvent;
		mev->MagellanButton = event.button.bnum;
	}

	return mev->MagellanType;
}

int MagellanRemoveMotionEvents(Display *dpy)
{
	return spnav_remove_events(SPNAV_EVENT_MOTION);
}


int MagellanRotationMatrix(double mat[4][4], double x, double y, double z)
{
	return 0;	/* TODO */
}

int MagellanMultiplicationMatrix(double mat_a[4][4], double mat_b[4][4], double mat_c[4][4])
{
	return 0;	/* TODO */
}
