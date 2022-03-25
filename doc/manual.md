libspnav manual
===============

Table of contents
-----------------
- [About libspnav](#about-libspnav)
- [Quick start](#quick-start)
- [Native libspnav API](#native-libspnav-api)
   - [Initialization and cleanup](#initialization-and-cleanup)
   - [Input events](#input-events)
   - [Device information](#device-information)
   - [Configuration management](#configuration-management)
- [Magellan API](#magellan-api)

About libspnav
--------------
Libspnav is a C library which can be used to communicate, and receive
6dof input events, from the free spacenav driver, or the proprietary
3dconnexion driver.

There are two modes of operation supported by libspnav. It can
communicate through a custom spacenav protocol, which works over UNIX
domain sockets, or it can communicate through the X11 magellan protocol
based on `ClientMessage` events. The first mode of operation is
only compatible with spacenavd, but provides extended functionality, and
does not need an X server to be running to function.  The second mode can
be used to talk to either spacenavd or the 3Dconnexion 3dxsrv driver, but
only supports a very limited subset of features, and depends on X11.

To be continued ...

