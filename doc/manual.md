libspnav manual
===============

Table of contents
-----------------
- [About libspnav](#about-libspnav)
- [Quick start](#quick-start)
- [Libspnav API](#libspnav-api)
   - [Initialization and cleanup](#initialization-and-cleanup)
   - [Application requests](#application-requests)
   - [Input events](#input-events)
   - [Device information](#device-information)
   - [Utility functions](#utility-functions)
   - [Configuration management](#configuration-management)
- [Magellan API](#magellan-api)

About libspnav
--------------
Libspnav is a C library for receiving input from 6 degrees-of-freedom (6dof)
input devices, also known as spacemice, spaceballs, etc. 6dof input is very
useful for fluidly manipulating objects or viewpoints in 3D space, and
fundamendally comprises of relative movement (translation) across 3 axes
(TX/TY/TZ), and rotation about 3 axes (RX/RY/RZ).

Libspnav is a counterpart to the free spacenav driver (spacenavd), which runs as
a system daemon, and handles all the low level interfacing with the actual
devices. However, it can also communicate with the proprietary 3Dconnexion
driver (3dxsrv), with reduced functionality.

There are two modes of operation supported by libspnav:

  1. It can use the native spacenav protocol, which works over UNIX domain
     sockets, supports the whole feature set exposed by libspnav, but is only
     compatible with spacenavd. This is used when `spnav_open` is called to
     connect to the driver.

  2. It can use the X11 magellan protocol, based on X `ClientMessage` events,
     which is compatible with both spacenavd and the proprietary 3dxsrv, but
     only supports a limited subset of features. Basically just receiving input
     events and changing sensitivity, no queries, device information, or
     configuration management. This is used when `spnav_x11_open` is called to
     connect to the driver.

Since the 3Dconnexion driver is practically unmaintained for the last decade,
and never really worked very well to begin with (which was what led to the
development of spacenavd), and also since the magellan protocol introduces an
unnecessary dependency to the X window system, and provides very rudimentary
functionality, new applications are encouraged to use `spnav_open`.

Also, libspnav provides a magellan API wrapper on top of the libspnav API, which
allows existing applications written for the official SDK to be transparently
recompiled against libspnav and be free of the 3Dconnexion licensing conditions.

Quick start
-----------
Libspnav, as of version 1.0 has been expanded, but really most applications will
only ever need to call a handful of functions to receive input events. A good
starting point is to read the heavily commented `spnav.h` header file, and the
simple example programs which come with libspnav, but this section will cover
the basics which most applications will need.

To use libspnav you need to include the header file `<spnav.h>` and link with
`-lspnav`.

The first thing to do, is to connect to the driver, which is done by calling
`spnav_open`:

    if(spnav_open() == -1) {
        /* failed to connect to spacenavd */
    }

When it's time to stop talking to spacenavd, call `spnav_close` to close the
connection.

After connecting you can go straight to your event loop, or you can query some
information about the device used by spacenavd:

    char buf[256];
    if(spnav_dev_name(buf, sizeof buf) != -1) {
        printf("Device: %s\n", buf);
    }
    num_buttons = spnav_dev_buttons();

If you only want to receive input from libspnav, or if you have a dedicated
thread for it, you can simply wait for events like so:

    spnav_event sev;
    while(spnav_wait_event(&sev)) {
        switch(sev.type) {
        case SPNAV_EVENT_MOTION:
            /* translation in sev.motion.x, sev.motion.y, sev.motion.z.
             * rotation in sev.motion.rx, sev.motion.ry, sev.motion.rz.
             */
            break;
        case SPNAV_EVENT_BUTTON:
            /* 0-based button number in sev.button.bnum.
             * button state in sev.button.press (non-zero means pressed).
             */
            break;
        }
    }

> Note that there is a set of helper utility functions provided by libspnav, for
> accumulating motion inputs into a position vector and an orientation quaternion,
> and for extracting a matrix from them suitable for manipulating 3D objects, or
> for manipulating the view to fly through a 3D scene. See the "cube" and "fly"
> example programs.

Alternatively an application can check for new events without blocking, by
calling `spnav_poll_event`. It's also worth noting that both functions work
exactly the same regardless of which protocol is in use.

Applications which need to integrate libspnav input to their main event loop,
can call `spnav_fd()` to retrieve the file descriptor corresponding to the
communication socket. This can be used in a regular `select` loop:

    fd_set rdset;
    int sock, count;
    spnav_event sev;

    FD_ZERO(&rdset);
    FD_SET(sock, &rdset);
    /* ... other file descriptors ... */

    if(select(maxfd + 1, &rdset, 0, 0, 0) <= 0) {
        return;
    }
    if(FD_ISSET(sock, &rdset)) {
        while(spnav_poll_event(&sev)) {
            /* ... handle spacenav event ... */
        }
    }

> The "fly" example program also demonstrates how to integrate libspnav events
> in a select loop, which is also handling X events coming from the Xlib socket.

The level of functionality described in this section (minus the device queries),
can be achieved and works exactly the same with both protocols. The only
difference is in calling `spnav_open` for the native protocol, or calling
`spnav_x11_open` and passing your `Window` id for the magellan X11 protocol. To
see the differences clearly, check out the "simple" example program, and look
for the conditionally compiled blocks of code marked with `#if
defined(BUILD_AF_UNIX)` and `#if defined(BUILD_X11)`.

Libspnav API
------------
This section will describe all the functions and structures in the libspnav API
in detail.

### Initialization and cleanup

#### spnav\_open

Function prototype: `int spnav_open(void)`

Open connection to the driver via the native spacenav protocol (`AF_UNIX` socket).
The native protocol is not compatible with the proprietary `3dxsrv` driver, but
provides the maximum level of functionality, and does not depend on X11 to
function. If you wish to remain compatible with `3dxsrv` with a reduced feature
set, use `spnav_x11_open` instead.

Returns: 0 on success, -1 on failure.

#### spnav\_close

Function prototype: `int spnav_close(void)`

Close the connection to the driver. No further events will be received.

Returns: 0 on success, -1 on failure.

#### spnav\_fd

Function prototype: `int spnav_fd(void)`

Returns the file descriptor of the socket used to communicate with the driver.
It can be used to integrate libspnav events in a standard select loop. If the
native protocol is in use, the spacenavd socket is returned. If the X11 magellan
protocol is in use, the socket used to communicate with the X server is
returned. In either case the file descriptor returned by `spnav_fd` can be used
to wait for events. If no connection has been established, or the connection has
been closed, -1 is returned.

Returns: file descriptor on success, -1 on failure.

#### spnav\_x11\_open

Function prototype: `int spnav_x11_open(Display *dpy, Window win)`

Open a connection to the driver via the X11 magellan protocol. Events are
delivered to the window registered by this call. More windows can be registered
by calling `spnav_x11_window`.

Returns: 0 on success, -1 on failure.

#### spnav\_protocol

Function prototype: `int spnav_protocol(void)`

Returns the spacenav protocol version used in the currently established
connection. If there is no established connection, or if the connection is using
the X11 magellan protocol, -1 is returned.

Returns: protocol version on success, -1 on failure.

### Application requests

#### spnav\_client\_name

Function prototype: `int spnav_client_name(const char *name)`

This is an optional request, which can be used to set the application name to
spacenavd. Currently it's only used for logging. Only works over the spacenav
protocol v1 or later.

Returns: 0 on success, -1 on failure.

#### spnav\_evmask

Function prototype: `int spnav_evmask(unsigned int mask)`

Select the types of events the application is interested in receiving. Available
options, which can be or-ed together, are:
  - `SPNAV_EVMASK_MOTION`: 6dof motion events (enabled by defualt).
  - `SPNAV_EVMASK_BUTTON`: button press/release events (enabled by default).
  - `SPNAV_EVMASK_DEV`: device change events (enabled by default for protocol v1 clients).
  - `SPNAV_EVMASK_CFG`: configuration change events.
  - `SPNAV_EVMASK_RAWAXIS`: raw device axis events (not useful for most apps).
  - `SPNAV_EVMASK_RAWBUTTON`: raw device button events (not useful for most apps).

Event mask selection only works over the spacenav protocol v1 or later.

Returns: 0 on success, -1 on failure.

#### spnav\_sensitivity

Function prototype: `int spnav_sensitivity(double sens)`

Sets the client sensitivity, which is used to scale all reported motion values
to this application.

Returns: 0 on success, -1 on failure.

#### spnav\_x11\_window

Function prototype: `int spnav_x11_window(Window win)`

For applications using the X11 magellan protocol (`spnav_x11_open`).
Register a window which will receive events by the driver.

Returns: 0 on success, -1 on failure.


### Input events

#### spnav\_wait\_event

Function prototype: `int spnav_wait_event(spnav_event *ev)`

Blocks waiting for spnav events. When an event is pending, it writes the event
through the `ev` pointer and returns a non-zero value. `spnav_event` is defined
as a union of structures, of all possible event types:

    typedef union spnav_event {
        int type;
        struct spnav_event_motion motion;
        struct spnav_event_button button;
        struct spnav_event_dev dev;
        struct spnav_event_cfg cfg;
        struct spnav_event_axis axis;
    } spnav_event;

See the heavily commented `spnav.h` file for details of all the different event
structures.

Returns: non-zero when it successfully received an event, 0 on failure.

#### spnav\_poll\_event

Function prototype: `int spnav_poll_event(spnav_event *ev)`

Checks if there is a pending spnav event without blocking.  If there are any
pending events, it removes one from the queue, writes it through the `ev`
pointer, and returns the event type. If there is no event pending, it returns 0.
See `spnav_wait_event` above for the definition of `spnav_event`.

Returns: event type if an event is pending, 0 if there are no available events.

#### spnav\_remove\_events

Function prototype: `int spnav_remove_events(int type)`

Drops any pending events of the specified type, or all pending events if
`SPNAV_EVENT_ANY` is passed.

Returns: number of events removed from the queue.

#### spnav\_x11\_event

Function prototype: `int spnav_x11_event(const XEvent *xev, spnav_event *sev)`

For applications using the X11 magellan protocol (`spnav_x11_open`).
Examines an X11 event to determine if it's an spnav event or not.

Returns: event type (`SPNAV_EVENT_MOTION`/`SPNAV_EVENT_BUTTON`) if it is an
spnav event, or 0 if it's not an spnav event.


### Device information

The functions in this section only work over the native spacenav protocol v1 or
later, and will fail if the connection is using the X11 magellan protocol, or
the spacenav protocol v0 (spacenavd <= 0.8).

#### spnav\_dev\_name

Function prototype: `int spnav_dev_name(char *buf, int bufsz)`

Retrieve a descriptive name for the currently active device used by spacenavd.
The name is written to the buffer passed as the first argument. No more than
`bufsz` bytes are written, including the zero terminator. The total length of
the complete device name is returned, even if it was truncated to fit in the
provided buffer. So to make sure you get the complete name you can use the
following code:

    int len = spnav_dev_name(0, 0);
    if(len > 0) {
        char *buf = malloc(len + 1);
        spnav_dev_name(buf, len + 1);
    }

Returns: length of the device name, or -1 on failure.

#### spnav\_dev\_path

Function protoype: `int spnav_dev_path(char *buf, int bufsz)`

Retrieve the path of the device file used by spacenavd to interface with the
actual device. See `spnav_dev_name` for a description of how retrieving strings
work in libspnav.

Returns: length of the path string, or -1 on failure.

#### spnav\_dev\_buttons

Function prototype: `int spnav_dev_buttons(void)`

Returns the number of physical buttons present on the currently used device. If
the query fails (due to incorrect protocol), the default value of 2 buttons is
returned.

Returns: number of device buttons, or 2 (default) on failure.

#### spnav\_dev\_axes

Function prototype: `int spnav_dev_axes(void)`

Returns the number of physical device axes on the currently used device. If the
query fails (due to incorrect protocol), the default value of 6 axes is
returned.

Returns: number of device axes, or 6 (default) on failure.

#### spnav\_dev\_usbid

Function prototype: `int spnav_dev_usbid(unsigned int *vendor, unsigned int *product)`

If the currently used device is a USB device, its USB vendor:product id is
written through the `vendor` and `product` pointers. Fails for non-USB devices,
of if no device is detected.

Returns: 0 on success, -1 on failure.

#### spnav\_dev\_type

Function prototype: `int spnav_dev_type(void)`

Returns the device type (see `SPNAV_DEV_*` enumeration in `spnav.h`) for the
currently used device. If the device is not known, `SPNAV_DEV_UNKNOWN` is
returned.

Returns: device type on success, -1 on failure.


### Utility functions

A number of helper utility functions for common tasks needed by most 3D programs
using 6dof input.

#### spnav\_posrot\_init

Function prototype: `void spnav_posrot_init(struct spnav_posrot *pr)`

The `spnav_posrot` structure is used by a set of helper functions to accumulate
motion input from spnav motion events, and convert the result into a matrix. It
contains a position vector, and a rotation quaternion:

    struct spnav_posrot {
        float pos[3];  /* position vector (x, y, z) */
        float rot[4];  /* orientation quaternion (x, y, z, w) w:real xyz:imaginary */
    };

This function initializes the vector to 0, and the quaternion to an identity
unit quaternion (1 + 0i + 0j + 0k), ready to start accumulating motion inputs.

#### spnav\_posrot\_moveobj

Function prototype: `void spnav_posrot_moveobj(struct spnav_posrot *pr, struct spnav_event_motion *ev)`

Apply position and rotation inputs from an spnav motion event, in a way suitable
for manipulating a 3D object. Use in conjunction with `spnav_matrix_obj` to
later extract a model matrix from the accumulated position/rotation.

#### spnav\_posrot\_moveview

Function prototype: `void spnav_posrot_moveview(struct spnav_posrot *pr, struct spnav_event_motion *ev)`

Apply position and rotation inputs from an spnav motion event, in a way suitable
for manipulating the view for flying through a 3D scene. Use in conjunction with
`spnav_matrix_view` to later extract a view matrix from the accumulated
position/rotation.

#### spnav\_matrix\_obj

Function prototype: `void spnav_matrix_obj(float *mat, struct spnav_posrot *pr)`

Construct a 4x4 homogeneous transformation matrix from the `spnav_posrot`
structure, suitable for use as a model/world matrix to position and orient a 3D
object. Use in conjunction with `spnav_posrot_moveobj` to accumulate motion
inputs.
The first argument is a pointer to an array of 16 floats, where the matrix is
written. The matrix is in the order expected by OpenGL.

#### spnav\_matrix\_view

Function prototype: `void spnav_matrix_view(float *mat, struct spnav_posrot *pr)`

Construct a 4x4 homogeneous transformation matrix from the `spnav_posrot`
structure, suitable for use as a view matrix for 6dof-controllef flight in 3D
space. Use in conjunction with `spnav_posrot_moveview` to accumulate motion
inputs.
The first argument is a pointer to an array of 16 floats, where the matrix is
written. The matrix is in the order expected by OpenGL.


### Configuration management

The configuration API is of no use to regular libspnav applications. It's only
intended for writing configuration management tools like the `spnavcfg` program.

It is useful to facilitate writing better configuration management programs,
which is why these calls are part of libspnav, but it would be quite rude
towards the user for most 3D applications to modify spacenavd settings through
this interface. Please think twice before calling any of the `spnav_cfg_*`
functions.

The configuration functions change the active settings in the currently running
spacenavd. To make the changes persistent, they should be written back to the
spacenavd configuration file, which is done by calling `spnav_cfg_save`.

Finally these functions, similarly to the device queries above, only work over
the native spacenav protocol v1 or later.

#### spnav\_cfg\_reset

Function prototype: `int spnav_cfg_reset(void)`

Reset all settings to their default values.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_restore

Function prototype: `int spnav_cfg_restore(void)`

Re-read the spacenavd configuration file, and revert all settings to their
values defined there.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_save

Function prototype: `int spnav_cfg_save(void)`

Save all the current settings to the spacenavd configuration file. This is
required to make any changes perists across spacenavd restarts.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_set\_sens

Function prototype: `int spnav_cfg_set_sens(float s)`

Set the global sensitivity for all motion axes (default: 1.0).

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_get\_sens

Function prototype: `float spnav_cfg_get_sens(void)`

Get the global sensitivity for all motion axes.

Returns: sensitivity on success, < 0.0 on failure

#### spnav\_cfg\_set\_axis\_sens

Function prototype: `int spnav_cfg_set_axis_sens(const float *svec)`

Set the per-axis sensitivity for all 6 input axes (TX, TY, TZ, RX, RY, RZ). The
function expects an array of 6 floats. The values are multiplied, so 1.0 means
default sensitivity, values higher than 1 increase sensitivity, while values
less than 1 decrease it.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_get\_axis\_sens

Function prototype: `int spnav_cfg_get_axis_sens(float *svec)`

Expects an array of 6 floats, and writes the current per-axis sensitivity values
for all 6 input axes there.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_set\_invert

Function prototype: `int spnav_cfg_set_invert(int invbits)`

Expects a bitmask where bits 0 to 5 define which of the 6 input axes to invert.
Bit 0 corresponds to TX, bit 1 to TY, and so on.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_get\_invert

Function prototype: `int spnav_cfg_get_invert(void)`

Returns a bitmask where bits 0 to 5 define which of the 6 input axes are
inverted. Bit 0 corresponds to TX, bit 1 to TY and so on.

Returns: invert bitmaks on success, -1 on failure.

#### spnav\_cfg\_set\_deadzone

Function prototype: `int spnav_cfg_set_deadzone(int devaxis, int delta)`

Sets the deadzone threshold (under which motion is considered to be noise and
discarded) for a specific *device axis*. Device axis numbers
range from 0 to the value returned by `spnav_dev_axes`-1, and are not affected
by remapping.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_get\_deadzone

Function prototype: `int spnav_cfg_get_deadzone(int devaxis)`

Returns the deadzone threshold for a specific *device axis*. Device axis numbers
range from 0 to the value returned by `spnav_dev_axes`-1, and are not affected
by remapping.

Returns: deadzone on success, -1 on failure.

#### spnav\_cfg\_set\_axismap

Function prototype: `int spnav_cfg_set_axismap(int devaxis, int map)`

Maps a *device axis* to an input axis. Both values are zero-based, device axes
range from 0 to the value returned by `spnav_dev_axes`-1, while input axes range
from 0 to 5 and correspond to the sequence: TX, TY, TZ, RX, RY, RZ. To unmap an
axis, you can map it to -1.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_get\_axismap

Function prototype: `int spnav_cfg_get_axismap(int devaxis)`

Returns the input axis mapping for the *device axis* `devaxis`. See
`spnav_cfg_set_axismap` for a description of axis numbers.

Returns: axis mapping on success, -1 on failure or if the axis in not mapped.

#### spnav\_cfg\_set\_bnmap

Function prototype: `int spnav_cfg_set_bnmap(int devbn, int map)`

Maps a *device button* to an input button. Both values range
from 0 to the value returned by `spnav_dev_buttons`-1. To unmap a button you can
map it to -1. Action mappings (`spnav_cfg_set_kbmap`) or keyboard mappings
(`spnav_cfg_set_bnaction`) for a given device button, override the regular
button mapping.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_get\_bnmap

Function prototype: `int spnav_cfg_get_bnmap(int devbn)`

Returns the input button mapped to the device button `devbn`. See
`spnav_cfg_set_bnmap` for a description of button numbers.

Returns: button mapping on success, -1 on failure or if unmapped.

#### spnav\_cfg\_set\_bnaction

Function prototype: `int spnav_cfg_set_bnaction(int devbn, int act)`

Maps a *device button* to an action. Device button numbers range from 0 to the
value returned by `spnav_dev_buttons`-1. Actions are one of the following:
  - `SPNAV_BNACT_NONE`: No action mapped, use as regular button (default).
  - `SPNAV_BNACT_SENS_RESET`: reset sensitivity to 1.
  - `SPNAV_BNACT_SENS_INC`: increase sensitivity.
  - `SPNAV_BNACT_SENS_DEC`: decrease sensitivity.
  - `SPNAV_BNACT_DISABLE_ROT`: disable rotation while held down.
  - `SPNAV_BNACT_DISABLE_TRANS`: disable translation while held down.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_get\_bnaction

Function prototype: `int spnav_cfg_get_bnaction(int devbn)`

Returns the action mapped to the device button `devbn`. See
`spnav_cfg_set_bnaction` for details.

Returns: action mapping on success, -1 on failure.

#### spnav\_cfg\_set\_kbmap

Function prototype: `int spnav_cfg_set_kbmap(int devbn, int key)`

Maps a *device button* to a keyboard key. Device button numbers range from 0 to
the value returned by `spnav_dev_buttons`-1. Key values are X11 `KeySym`s
defined in `X11/keysymdef.h`. You can use `XStringToKeysym` and
`XKeysymToString` to convert between a key name and a keysym if necessary.

Keyboard mappings only work spacenavd is compiled with X11 support (which is the
default), and will be unreliable if spacenavd is compiled without XTEST support.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_get\_kbmap

Function prototype: `int spnav_cfg_get_kbmap(int devbn)`

Returns the X11 keysym mapped to the device button `devbn`. See
`spnav_cfg_set_kbmap` for details.

Returns: keysym mapping on success, -1 on failure.

#### spnav\_cfg\_set\_swapyz

Function prototype: `int spnav_cfg_set_swapyz(int swap)`

Enable or disable swapping of the translation and rotation Y and Z axes.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_get\_swapyz

Function prototype: `int spnav_cfg_get_swapyz(void)`

Returns: 0 if the Y-Z axes are not swapped, 1 if they are, -1 on failure.

#### spnav\_cfg\_set\_led

Function prototype: `int spnav_cfg_set_led(int mode)`

Sets the LED mode for devices with LEDs around the puck. The mode value can be:
  - `SPNAV_CFG_LED_OFF`
  - `SPNAV_CFG_LED_ON`
  - `SPNAV_CFG_LED_AUTO`

In "auto" mode, the LED is off by default, and turns on when one or more clients
are connected to spacenavd.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_get\_led

Function prototype: `int spnav_cfg_get_led(void)`

Returns the LED mode for devices with LEDs around the puck. See
`spnav_cfg_set_led` for the possible modes.

Returns: LED mode on success, -1 on failure.

#### spnav\_cfg\_set\_grab

Function prototype: `int spnav_cfg_set_grab(int grab)`

Enable or disable exclusive device access. When grab is enabled, no other
program can use the device. This is useful for stopping the X server for trying
to use it as a regular mouse, but will interfere with programs which attempt to
use 6dof devices directly, like google earth. Pass non-zero to enable the
exclusive access grab setting, or 0 to disable it.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_get\_grab

Function prototype: `int spnav_cfg_get_grab(void)`

Returns the exclusive device access grab setting. See `spnav_cfg_set_grab` for
details.

Returns: exclusive device grab setting on success, -1 on failure.

#### spnav\_cfg\_set\_serial

Function prototype: `int spnav_cfg_set_serial(const char *devpath)`

Set the device file path to use for opening serial 6dof devices. Serial devices
cannot be detected automatically, the user needs to provide the device path
explicitly. To not attempt to open a serial device, pass an empty string.

Note: for security reasons, the path set through this setting must correspond to
a TTY character device, otherwise it is ignored.

Returns: 0 on success, -1 on failure.

#### spnav\_cfg\_get\_serial

Function prototype: `int spnav_cfg_get_serial(char *buf, int bufsz)`

Retrieve the currently configured serial device path. See `spnav_dev_name` for a
description of how to use string query functions like this.

Returns: serial device path length on success, -1 on failure.


Magellan API
------------

No detailed documentation is currently available for the Magellan API wrapper.
It is however rather simple, so go ahead and browse through the
`spnav_magellan.h` header file if you want to know more.

New programs are discouraged from using the Magellan API wrapper. Everything
that can be done with it, can also be done with the regular libspnav API; it is
after all just as thin compatibility layer on top of the libspnav API.

It is **not** necessary to use the Magellan API to achieve compatibility with
the proprietary 3Dconnexion driver; using the libspnav API with `spnav_x11_open`
makes your program compatible with it.
