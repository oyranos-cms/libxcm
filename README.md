# libXcm README
[![Build Status](https://travis-ci.org/oyranos-cms/libxcm.svg?branch=master)](https://travis-ci.org/oyranos-cms/libxcm)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/4303/badge.svg)](https://scan.coverity.com/projects/4303)
[![Documentation](https://codedocs.xyz/oyranos-cms/libxcm.svg)](https://codedocs.xyz/oyranos-cms/libxcm/)
[![License](https://img.shields.io/github/license/mashape/apistatus.svg)](http://www.opensource.org/licenses/mit-license.php)

The libXcm library contains the a reference implementation of the 
X Color Management specification. The X Color Management specification 
allows to attach colour regions to X windows to communicate with colour
servers.

The XcmDDC API can be used to fetch a EDID data block from a monitor over
a i2c communication. On Linux typical the i2c-dev module
must be loaded to use this hardware data channel. The device nodes
should obtain rights accessible to normal users. The package config info is
in xcm-ddc.

The XcmEdidParse API is for parsing EDID data blocks. A small example 
application is included. The package config info is in xcm-edid.

The XcmEvents API allowes to easily observe X11 colour management events.
The included xcmsevents makes use of the XcmEvents API. The package config
info is in xcm-x11.


### Links
* sources: [git clone git@gitlab.com:oyranos/libxcm](https://gitlab.com/oyranos/libxcm)
* www: [www.oyranos.org/libxcm](http://www.oyranos.org/libxcm)
* support: [email list](http://lists.freedesktop.org/mailman/listinfo/openicc)
* [ChangeLog](docs/ChangeLog.md)
* [Copyright](docs/COPYING.md) - MIT
* [Authors](docs/AUTHORS.md)


### Dependencies - optional
* [Xorg](http://www.x.org)
* autotools-dev
* pkg-config
* libxfixes-dev
* x11proto-xext-dev


### Building
    $ make
    $ make install


