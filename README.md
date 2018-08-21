libspnav
========

About
-----

The libspnav library is provided as a replacement of the magellan library. It
provides a cleaner, and more orthogonal interface. libspnav supports both the
original X11 protocol for communicating with the driver, and the new
alternative non-X protocol. Programs that choose to use the X11 protocol, are
automatically compatible with either the free spacenavd driver or the official
3dxserv, as if they were using the magellan SDK.

Also, libspnav provides a magellan API wrapper on top of the new API. So, any
applications that were using the magellan library, can switch to libspnav
without any changes. And programmers that are familliar with the magellan API
can continue using it with a free library without the restrictions of the
official SDK.


Installation
------------

Configure, make, and make install as usual.


License
-------

Copyright (C) 2007-2018 John Tsiombikas <nuclear@member.fsf.org>

libspnav is free software. Feel free to use, modify, and/or redistibute it
under the terms of the 3-clause BSD license. See LICENSE for details.
