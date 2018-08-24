/*
This file is part of libspnav, part of the spacenav project (spacenav.sf.net)
Copyright (C) 2007-2010 John Tsiombikas <nuclear@member.fsf.org>

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

/* spnav_magellan.h and spnav_magellan.c provide source-level compatibility
 * with the original proprietary Magellan SDK provided by 3Dconnexion.
 * It is actually implemented as a wrapper on top of the new spnav interface
 * (see spnav.h).
 */
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
		mev->type = event.button.press ? MagellanInputButtonPressEvent : MagellanInputButtonReleaseEvent;
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
		mev->MagellanType = event.button.press ? MagellanInputButtonPressEvent : MagellanInputButtonReleaseEvent;
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
