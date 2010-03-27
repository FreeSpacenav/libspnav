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
#ifndef SPACENAV_MAGELLAN_H_
#define SPACENAV_MAGELLAN_H_

#include <X11/Xlib.h>

enum {
	MagellanInputMotionEvent = 1,
	MagellanInputButtonPressEvent = 2,
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
