/*
This file is part of libspnav, part of the spacenav project (spacenav.sf.net)
Copyright (C) 2007-2022 John Tsiombikas <nuclear@member.fsf.org>

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

#include "spnav_config.h"

#ifdef SPNAV_USE_X11
#include <X11/Xlib.h>
#endif

enum {
	SPNAV_EVENT_ANY,	/* used by spnav_remove_events() */
	SPNAV_EVENT_MOTION,
	SPNAV_EVENT_BUTTON,	/* includes both press and release */

	SPNAV_EVENT_DEV,	/* add/remove device event */
	SPNAV_EVENT_CFG,	/* configuration change event */

	SPNAV_EVENT_RAWAXIS,
	SPNAV_EVENT_RAWBUTTON
};

enum { SPNAV_DEV_ADD, SPNAV_DEV_RM };

struct spnav_event_motion {
	int type;			/* SPNAV_EVENT_MOTION */
	int x, y, z;
	int rx, ry, rz;
	unsigned int period;
	int *data;
};

struct spnav_event_button {
	int type;			/* SPNAV_EVENT_BUTTON or SPNAV_EVENT_RAWBUTTON */
	int press;
	int bnum;
};

struct spnav_event_dev {
	int type;			/* SPNAV_EVENT_DEV */
	int op;				/* SPNAV_DEV_ADD / SPNAV_DEV_RM */
	int id;
	int devtype;		/* see spnav_dev_type() */
	int usbid[2];		/* USB id if it's a USB device, 0:0 if it's a serial device */
};

struct spnav_event_cfg {
	int type;			/* SPNAV_EVENT_CFG */
	int cfg;			/* same as protocol REQ_GCFG* enum */
	int data[6];		/* same as protocol response data 0-5 */
};

struct spnav_event_axis {
	int type;			/* SPNAV_EVENT_RAWAXIS */
	int idx;			/* axis number */
	int value;			/* value */
};

typedef union spnav_event {
	int type;
	struct spnav_event_motion motion;
	struct spnav_event_button button;
	struct spnav_event_dev dev;
	struct spnav_event_cfg cfg;
	struct spnav_event_axis axis;
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
 * no connection is open / failure occurred.
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




#ifdef SPNAV_USE_X11
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

/* returns the protocol version understood by the running spacenavd
 * -1 on error, or if the connection was established using the X11
 * protocol (spnav_x11_open).
 */
int spnav_protocol(void);


/*
 * Everything from this point on will either fail, or return hardcoded default
 * values when communicating over the X11 protocol (spnav_x11_open), or when
 * using a spacenav protocol version less than 1. See spnav_protocol().
 */

/* Set client name */
int spnav_client_name(const char *name);

/* Select the types of events the client is interested in receiving */
enum {
	SPNAV_EVMASK_MOTION		= 0x01,	/* 6dof motion events */
	SPNAV_EVMASK_BUTTON		= 0x02,	/* button events */
	SPNAV_EVMASK_DEV		= 0x04,	/* device change events */
	SPNAV_EVMASK_CFG		= 0x08,	/* configuration change events */

	SPNAV_EVMASK_RAWAXIS	= 0x10,	/* raw device axis events */
	SPNAV_EVMASK_RAWBUTTON	= 0x20,	/* raw device button events */

	SPNAV_EVMASK_INPUT		= SPNAV_EVMASK_MOTION | SPNAV_EVMASK_BUTTON,
	SPNAV_EVMASK_DEFAULT	= SPNAV_EVMASK_INPUT | SPNAV_EVMASK_DEV,
	SPNAV_EVMASK_ALL		= 0xffff
};
int spnav_evmask(unsigned int mask);

/* TODO multi-device support and device selection not implemented yet
int spnav_num_devices(void);
int spnav_select_device(int dev);
*/

/* Returns a descriptive device name.
 * If buf is not null, the name is copied into buf. No more than bufsz bytes are
 * written, including the zero terminator.
 * The number of bytes that would have been written assuming enough space in buf
 * are returned, excluding the zero terminator.
 */
int spnav_dev_name(char *buf, int bufsz);

/* Returns the path to the device file if applicable.
 * Usage same as spnav_dev_name.
 */
int spnav_dev_path(char *buf, int bufsz);

/* returns the number of buttons (defaults to 2 if unable to query) */
int spnav_dev_buttons(void);
/* returns the number of axes (defaults to 6 if unable to query) */
int spnav_dev_axes(void);

/* Writes the USB vendor:device ID through the vend/prod pointers.
 * Returns 0 for success, or -1 on failure (for instance if there is no open
 * device, or if it's not a USB device).
 */
int spnav_dev_usbid(unsigned int *vend, unsigned int *prod);

enum {
	SPNAV_DEV_UNKNOWN,
	/* serial devices */
	SPNAV_DEV_SB2003 = 0x100,	/* Spaceball 1003/2003/2003C */
	SPNAV_DEV_SB3003,			/* Spaceball 3003/3003C */
	SPNAV_DEV_SB4000,			/* Spaceball 4000FLX/5000FLX */
	SPNAV_DEV_SM,				/* Magellan SpaceMouse */
	SPNAV_DEV_SM5000,			/* Spaceball 5000 (spacemouse protocol) */
	SPNAV_DEV_SMCADMAN,			/* 3Dconnexion CadMan (spacemouse protocol) */
	/* USB devices */
	SPNAV_DEV_PLUSXT = 0x200,	/* SpaceMouse Plus XT */
	SPNAV_DEV_CADMAN,			/* 3Dconnexion CadMan (USB version) */
	SPNAV_DEV_SMCLASSIC,		/* SpaceMouse Classic */
	SPNAV_DEV_SB5000,			/* Spaceball 5000 (USB version) */
	SPNAV_DEV_STRAVEL,			/* Space Traveller */
	SPNAV_DEV_SPILOT,			/* Space Pilot */
	SPNAV_DEV_SNAV,				/* Space Navigator */
	SPNAV_DEV_SEXP,				/* Space Explorer */
	SPNAV_DEV_SNAVNB,			/* Space Navigator for Notebooks */
	SPNAV_DEV_SPILOTPRO,		/* Space Pilot pro */
	SPNAV_DEV_SMPRO,			/* SpaceMouse Pro */
	SPNAV_DEV_NULOOQ,			/* NuLOOQ */
	SPNAV_DEV_SMW,				/* SpaceMouse Wireless */
	SPNAV_DEV_SMPROW,			/* SpaceMouse Pro Wireless */
	SPNAV_DEV_SMENT,			/* SpaceMouse Enterprise */
	SPNAV_DEV_SMCOMP,			/* SpaceMouse Compact */
	SPNAV_DEV_SMMOD				/* SpaceMouse Module */
};

/* Returns the device type (see enumeration above) or -1 on failure */
int spnav_dev_type(void);


/* Utility functions
 * -----------------------------------------------------------------------------
 * These are optional helper functions which perform tasks commonly needed by 3D
 * programs using 6dof input.
 */

struct spnav_posrot {
	float pos[3];	/* position vector (x, y, z) */
	float rot[4];	/* orientation quaternion (x, y, z, w) w:real xyz:imaginary */
};

void spnav_posrot_init(struct spnav_posrot *pr);
void spnav_posrot_moveobj(struct spnav_posrot *pr, const struct spnav_event_motion *ev);
void spnav_posrot_moveview(struct spnav_posrot *pr, const struct spnav_event_motion *ev);

/* Construct a 4x4 homogeneous transformation matrix from the `spnav_posrot`
 * structure, suitable for use as a model/world matrix to position and orient a
 * 3D object. Use in conjunction with `spnav_posrot_moveobj` to accumulate
 * motion inputs.
 * The first argument is a pointer to an array of 16 floats, where the matrix is
 * written. The matrix is in the order expected by OpenGL.
 */
void spnav_matrix_obj(float *mat, const struct spnav_posrot *pr);

/* Construct a 4x4 homogeneous transformation matrix from the `spnav_posrot`
 * structure, suitable for use as a view matrix for 6dof-controllef flight in 3D
 * space. Use in conjunction with `spnav_posrot_moveview` to accumulate motion
 * inputs.
 * The first argument is a pointer to an array of 16 floats, where the matrix is
 * written. The matrix is in the order expected by OpenGL.
 */
void spnav_matrix_view(float *mat, const struct spnav_posrot *pr);


/* Configuration API
 * -----------------------------------------------------------------------------
 * This API is mostly targeted towards configuration management tools (like
 * spnavcfg). Normal clients should avoid changing the spacenavd configuration.
 * They should use the non-persistent client-specific settings instead.
 *
 * All functions with return 0 on success, -1 on failure, unless noted otherwise
 */

enum { SPNAV_CFG_LED_OFF, SPNAV_CFG_LED_ON, SPNAV_CFG_LED_AUTO };
enum {
	SPNAV_BNACT_NONE,
	SPNAV_BNACT_SENS_RESET, SPNAV_BNACT_SENS_INC, SPNAV_BNACT_SENS_DEC,
	SPNAV_BNACT_DISABLE_ROT, SPNAV_BNACT_DISABLE_TRANS,
	SPNAV_MAX_BNACT
};

/* Reset all settings to their default values. This change will not persist
 * between spacenavd restarts, unless followed by a call to spnav_cfg_save.
 */
int spnav_cfg_reset(void);

/* Revert all the session settings to the values defined in the spacenavd
 * configuration file. This change will not persist between spacenavd restarts,
 * unless followed by a call to spnav_cfg_save.
 */
int spnav_cfg_restore(void);

/* Save all the current settings to the spacenavd configuration file. */
int spnav_cfg_save(void);


/* Set the global sensitivity.
 * cfgfile option: sensitivity
 */
int spnav_cfg_set_sens(float s);
float spnav_cfg_get_sens(void);	/* returns the sensitivity or <0.0 on error */

/* Set the per-axis sensitivity for all 6 axes. svec should point to an array of
 * 6 floats.
 * cfgfile options: sensitivity-translation-[x|y|z], sensitivity-rotation-[x|y|z]
 */
int spnav_cfg_set_axis_sens(const float *svec);
int spnav_cfg_get_axis_sens(float *svecret);  /* writes 6 floats through svec */

/* Set deadzone for a device axis
 * cfgfile options: dead-zoneN,
 */
int spnav_cfg_set_deadzone(int devaxis, int delta);
int spnav_cfg_get_deadzone(int devaxis);	/* returns deadzone, -1 on error */

/* Set the axis invert state
 * 0: normal, 1: inverted. order: MSB [ ... RZ|RY|RX|TZ|TY|TX] LSB
 * cfgfile options: invert-trans, invert-rot
 */
int spnav_cfg_set_invert(int invbits);
int spnav_cfg_get_invert(void);	/* returns invert bits, -1 on error */

int spnav_cfg_set_axismap(int devaxis, int map);
int spnav_cfg_get_axismap(int devaxis);

int spnav_cfg_set_bnmap(int devbn, int map);
int spnav_cfg_get_bnmap(int devbn);

int spnav_cfg_set_bnaction(int devbn, int act);
int spnav_cfg_get_bnaction(int devbn);

int spnav_cfg_set_kbmap(int devbn, int key);
int spnav_cfg_get_kbmap(int devbn);

int spnav_cfg_set_swapyz(int swap);
int spnav_cfg_get_swapyz(void);

/* Control device LED
 * SPNAV_CFG_LED_OFF | SPNAV_CFG_LED_ON | SPNAV_CFG_LED_AUTO
 * cfgfile option: led
 */
int spnav_cfg_set_led(int state);
int spnav_cfg_get_led(void);	/* returns led setting, -1 on error */

/* Device exclusive access grabbing */
int spnav_cfg_set_grab(int state);
int spnav_cfg_get_grab(void);	/* returns 0:no-grab/1:grab, or -1 on error */

int spnav_cfg_set_serial(const char *devpath);
int spnav_cfg_get_serial(char *buf, int bufsz);

#ifdef __cplusplus
}
#endif

#endif	/* SPACENAV_H_ */
