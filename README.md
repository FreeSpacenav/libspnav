libspnav
========

About
-----
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

Also, libspnav provides a magellan API wrapper on top of the libspnav API, which
allows existing applications written for the official SDK to be transparently
recompiled against libspnav and be free of the 3Dconnexion licensing conditions.

Documentation
-------------
To learn how to use libspnav in your programs, refer to the [manual](doc/manual.md),
which is available in markdown format under the `doc` directory of the libspnav
distribution. The manual is also available online in HTML format on the free
spacenav project website: http://spacenav.sourceforge.net/man_libspnav

Also make sure to check out the example programs which come with libspnav under
the `examples` directory.

![examples](http://spacenav.sourceforge.net/images/exbar.png)


Installation
------------
To build and install libspnav, simply run:

    ./configure
    make
    make install

Most likely the `make install` part will need to be executed as root, if you're
installing libspnav system-wide, which is the common case
(default prefix is: `/usr/local`).

Running `./configure --help` prints available build options.


License
-------
Copyright (C) 2007-2022 John Tsiombikas <nuclear@member.fsf.org>

libspnav is free software. Feel free to use, modify, and/or redistibute it
under the terms of the 3-clause BSD license. See LICENSE for details.
