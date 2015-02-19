# libXcm

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


## Internet
* sources: [git clone git://github.com/oyranos-cms/libxcm](https://github.com/oyranos-cms/libxcm)
* www: [OpenICC](http://www.oyranos.org/libxcm)
* support: [email list](http://lists.freedesktop.org/mailman/listinfo/openicc)


## Dependencies
[Xorg](http://www.x.org)
libxmu-dev


## Building
    $ make
    $ make install

