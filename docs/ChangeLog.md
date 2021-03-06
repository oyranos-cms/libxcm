# ChangeLog

# Version 0.5.4

### Kai-Uwe Behrmann (43):
* [core]: fix some compile warnings
* [core]: show _NET_DESKTOP_GEOMETRY event
* [docs]: repair github markdown
* [docs]: convert more docs to markdown
* [docu]: add README badges for issues and license
* [docu]: move the project links below the headline
* [docu]: link to development page
* [docu]: add link to life doxygen
* [conf]: exclude more intermediate build files from git
* [core]: allow only printable ascii for edid strings
* [core]: fix newly introduced edid vendor parsing error
* [conf]: fix autotools rebuilding scripts for CI
* [conf]: add missed autotools file
* [conf]: fix distcheck target
* [docu]: declare most dependencies optional
* [core]: fix warning: iteration 1u invokes undefined behavior [-Waggressive-loop-optimizations]
* [conf]: file shuffling
* [docu]: slightly simplify tutorial compile option
* [build]: update all autotools files
* [conf]: sync local spec file almost with OBS one
* [build]: update autotools for README.md rename
* [build]: use orig.tar.bz2 for rpm target
* [docu]: remove unused file
* Revert "* [docu]: satisfy autotools"
* [docu]: satisfy autotools
* [docu]: improve html docu
* [core]: fix Dereference after null check
* [core]: fix use of correct deallocator
* [core]: fix warning about getc truncation
* [core]: fix possiply deferencing null pointer
* [docu]: explain XcmDummy function
* [core]: fix introduces access of uninitialised pointer
* [core]: fix CID 31728 resource leak
* [core]: fix some coverity detected leakages
* [core]: more clearly set char and not pointer
* [conf]: add coverty key
* [docu]: add CI status label to README
* [conf]: add CI dependencies
* [docu]: convert README to markdown format
* [conf]: enable CI
* [docu]: update URLs
* [conf]: bump version
* [build]: fix RPM spec requires

# Version libXcm-0.5.3

### Florian Höch (1):
	* [core]: Patch for transposed EDID red y bits

### Kai-Uwe Behrmann (20):
	* [build]: remove all libtool files from RPM
	* [core]: fix conditional compiling of XcmDummy
	* [build]: fix spec packaging issues
	* [core]: call split libraries in libXcm
	* [build]: sync spec file with obs
	* [core]: add XcmDummy.c file
	* [build]: add xcm-edid xcm-x11 xcm-ddc pkg-config files
	* [build]: reorder building of libraries
	* [build]: split EDID parsing library out
	* [core]: XcmeContext_InLoop return meaningful value
	* [core]: Fix window name print
	* [conf]: update debian changelog
	* [docu]: update man pages
	* [core]: omit XmuClientWindow()
	* [conf]: bump version
	* [core]: initialy observe windows
	* [docu]: fix link
	* [build]: update autotools files
	* [build]: remove libtool
	* [build]: add missed automake script

# Version libXcm-0.5.2

### Jan Engelhardt (2):
	build: support for automake-1.12
	build: do not fail if AM_SILENT_RULES is not present

### Jean-Sébastien Pédron (2):
	* [build]: support build dir != source dir
	* [build]: regen autotools files to correctly support FreeBSD 10

### Kai-Uwe Behrmann (11):
	* [build]: rename FindXcm.cmake to XcmConfig.cmake
	* [conf]: install FindXcm.cmake
	* [core]: define XCM_COLOR_SERVER_MANAGEMENT enum
	* [docs]: describe ICM in X Color Management spec
	* [build]: fix debian install file names
	* [build]: add needed build requirements for debian
	* [build]: add requred dummy file
	* [build]: add make debian target
	* [docu]: mention opt out in X Color Management spec
	* [conf]: bump version
	* [build]: reset release version in RPM spec file

# Version libXcm-0.5.1

### Kai-Uwe Behrmann (4):
	* [core]: add XCM_ prefix to HAVE macros
	* [core]: simplify varg allocation for messages
	* [build]: depend on dist-bzip2 for make rpm target
	* [conf]: fix DDC code linking

### Michael Weber (1):
	* [conf]: add --disable-x11 configure option


# Version libXcm-0.5.0

### Jan Engelhardt (7):
	* [conf]: allow building without X11
	* [build]: allow building without X11
	* [core]: resolve compiler warnings about incorrect code
	* [conf]: Xcm header files utilize header files from xproto (Xatom.h) and x11 (Xlib.h)
	* [core]: resolve compiler warnings about incorrect code
	* [conf]: switch to autotools
	* [conf]: Xcm header files utilize header files from xproto (Xatom.h) and x11 (Xlib.h)

### Kai-Uwe Behrmann (75):
	* [build]: correct configure version macor in spec
	* [build]: update spec file for autotools changes
	* [core]: keep capabilities visible for non X11
	* [build]: add missed autotools files
	* [core]: skip "WARNING" for system message in xcmevents
	* [conf]: add xproto x11 conditionally to xcm.pc
	* [conf]: HAVE_X11 conditional for Xcm.h
	* [conf]: fix XcmEvents building
	* [conf]: copy build system
	* [core]: add XCM_COLOR_OUTPUTS to header
	* [docu]: adapt the X Color Management spec intro
	* [docu]: X Color Management specification 0.4
	* [conf]: fix Version field
	* [conf]: add Version field to pkg-config file
	* [conf]: fill all needed paths for pkg-config
	* [conf]: add config.h.in
	* [conf]: add build-aux files
	* [conf]: track essential build scripts and files
	* [conf]: remove old configuration files
	* [conf]: add status at end of configure run
	* [build]: add example files to package
	* [build]: add make help target
	* [build]: add make rpm and deb targets
	* [docu]: fix doxygen warning
	* [core]: use XcmVersion.h macros
	* [conf]: write HAVE_X11 to XcmVersion.h
	* [conf]: set fine grained package and library verions
	* [build]: pass libtool version to linker
	* [build]: readd missed files to tar ball
	* [conf]: adapt spec file template to autotools
	* [docu]: show macros for doxygen man pages
	* [conf]: add version header template
	* [conf]: add urls to configure.ac
	* [conf]: prepare autotools patch
	* [conf]: generalise Win32 detection
	* [conf]: check for MINGW32
	* [conf]: always use -fPIC
	* [core]: print ser_id
	* [core]: fix ser_id field to 32bit
	* [core]: add XcmColorServerCapabilities()
	* [core]: fix colorimetry name macros
	* [core]: fix string buffer allocation
	* [core]: fix key name swapping
	* [docu]: X Color Management v0.3 DRAFT2
	* [core]: adapt new XCM_COLOUR_DESKTOP_ADVANCED macro
	* [core]: detect _ICC_COLOR_DISPLAY_ADVANCED events
	* [docu]: add _ICC_COLOR_DISPLAY_ADVANCED + _ICC_COLOR_PROFILES
	* [core]: add a name to the XcmEvents observer window
	* [core]: add XcmeContext_Setup2 API
	* [core]: add SEC manufacturer
	* [core]: add GCM and LGE
	* [core]: be more strict about EDID primariy order
	* [core]: add LGD manufacturer
	* [conf]: bump to 0.5
	* [docu]: update to new XCM macro
	* [build]: fix spec file
	* [core]: use XCM_COLOR_ macros
	* [core]: device XCM atom strings
	* [docu]: correct the capabilities tagging XCM docu
	* [core]: rename net-color spec to X_Color_Management
	* [core]: export numbers as strings for JSON
	* [docu]: update autogenerated man pages
	* [exmpl]: use XcolorRegionDelete for 0 0 0 0 geometry
	* [core]: properly export XcmePrintWindowName
	* [build]: add client example make check target
	* [exmpl]: make profile selectable
	* [build]: add build line for color
	*  [exmpl]: compile color with C
	* [exmpl]: fix client
	* [build]: adapt spec file to openSUSE conventions
	* [exmpl]: add very simple makefile for color example
	* [core]: return Xorg error values
	* [exmpl]: update to actual library
	* [conf]: bump version
	* [core]: chose first text block for type

# Version libXcm-0.4.2

### Kai-Uwe Behrmann (11):
	* [core]: use only JSON array in xcmedid
	* [core]: skip the numbered JSON device level in xcmedid
	* [build]: default to bz2 compression for RPM
	* [build]: remove unused macros in RPM spec file
	* [core]: merge EDID week and year to EDID_date
	* [core]: fall back for EDID_model to model_id
	* [docu]: update XcmEdidParse man page
	* [core]: add AUO and LEN manufacturers
	* [core]: add XcmEdidPrintOpenIccJSON() API
	* [core]: exclude DDC from __FreeBSD__
	* [conf]: bump version

# Version libXcm-0.4.1

### Kai-Uwe Behrmann (14):
	* [core]: dont call exit() from library code
	* [build]: rpm group is System/Libraries
	* [core]: initialy print capabilities
	* [core]: fix typo
	* [exmpl]: add net-color spec opt-out example
	* [build]: compress package with bzip2
	* [core]: disable ddc on apple
	* [conf]: sync test script with Oyranos
	* [docu]: fix some doxygen warnings
	* [conf]: use -fPIC only on 64bit builds
	* [docu]: update man page
	* [conf]: bump version
	* [build]: remove libxorg-x11-devel from mandriva
	* [docu]: update ChangeLog

### Nicolas Chauvet (1):
	* [conf]: apply Fedora patch for pkg-config

# Version libXcm-0.4.0

### Kai-Uwe Behrmann (26):
	* [conf]: bump version
	* [core]: fix empty string access in XcmStringAdd_
	* [core]: add XcmValueInt16()
	* [core]: add base EDID CMD block
	* [core]: allow for non X11 builds
	* [conf]: update month
	* [core]: fix compile warnings
	* [docu]: add macros to man page
	* [core]: add more manufacturers and correct
	* [conf]: bump version to 0.4.0
	* [docu]: complete header information
	* [core]: add i2c EDID fetching
	* [docu]: build man pages for new API
	* [build]: cover the DDC files
	* [docu]: add XcmDDC man page
	* [core]: fix source file permissions
	* [core]: fix compile issues for non c99
	* [core]: fix c99 compile warnings
	* [core]: fix new DDC API and XcmDDCgetEDID()
	* [build]: omit c99 gcc flag
	* [docu]: update XcmDDC API
	* [exmpl]: remove xcmevents, it is in Xcm package
	* [exmpl]: move compiz code to examples
	* [exmpl]: move compiz code to examples
	* [docu]: update to XcmDDC
	* [build]: Makefile.all is no more up to date

# libXcm-0.3.0

### Kai-Uwe Behrmann (17):
	* [docu]: update ChangeLog
	* [conf]: bump to 0.2.8
	* [docu]: Draft 2 for net-color spec 0.2
	* [core]: add C++ guards, add MIT license
	* [core]: add XcmEdidParse and XcmEvents APIs
	* [docu]: update to new APIs
	* [docu]: add license to Xcm.c
	* [core]: add missed header
	* [core]: add xcmsevents tool from Oyranos
	* [tools]: add edid-parse tool/example
	* [exmpl]: move edid-parse to own directory
	* [exmpl]: directory move
	* [build]: source package edid-parse example
	* [build]: correct man page source packaging
	* [docu]: describe EDID key's
	* [docu]: update XcmEdidParse man page
	* [docu]: update AUTHORS and README

# libXcm-0.2.7

### Kai-Uwe Behrmann (10):
	* [docu]: limit doxygen search paths
	* [docu]: update ChangeLog
	* [doku]: rename Doxyfile for automatic updates
	* [conf]: generate Doxyfile
	* [build]: add install-docs, install-bin targets
	* [docu]: update man pages
	* [conf]: distribute Doxyfile.in
	* [conf]: bump version
	* [build]: sync with Nicolas Chauvet alias kwizart
	* [docu]: rename Xcm.3x to libXcm.3

# libXcm-0.2.6

### Kai-Uwe Behrmann (39):
	* [build]: link -lX11 to libXcolor.so
	* [Xc]: XcolorRegionDelete allow real delete
	* [Xc]: fix wrong memcpy bug
	* [color]: add device link support
	* [color] fix compilation for compiz-0.8.4
	* [exmpl]: fix some bugs
	* [build]: use pkg-config for lcms detection
	* [docu]: Draft 1 for net-color spec 0.2
	* [docu]: add distribution files
	* [build]: add distribution Makefile
	* [docu]: minor clearification
	* [docu]: fix typo
	* [build]: support CFLAGS LDFLAGS
	* [build]: rename sources
	* [docu]: refere to ICC for MD5 computation
	* [build]: rename old Makefile
	* [conf]: support pkgconfig, RPM, configure
	* [docu]: clearify description
	* [docu]: add man pages
	* [Xc]: sync endianess handling
	* [docu]: better formatting
	* [build]: add man pages to RPM
	* [build]: distribute and install man pages
	* [docu]: reference other man pages
	* [conf]: remove unneeded check
	* [build]: set local include path
	* [conf]: bump to 0.2.3
	* [build]: link again to X11
	* [conf]: rename to libXcm
	* [build]: fix the debian package name
	* [build]: install header into X11/; rename pkgcfg
	* [docu]: add Doxyfile
	* [conf]: show actual includedir
	* [build]: fix for mandriva 2007
	* [build]: RPM spec file fixes
	* [build]: correct soname
	* [build]: add requirement for RPM devel package
	* [build]: omit RPM buildroot cleaning
	* [conf]: bump version

### Tomas Carnecky (41):
	Initial import.
	Better visualization of the color transformation.
	Use the compiz fragment API
	Use stencil buffer instead of scissors. This fixes the problem with the wobbly plugin.
	Using dynamic rectangles now. Added simple client that shows communication between application and compositing manager.
	Fix potential crash
	Fetch all regions from the window property and use all of those.
	addDataOpToFunctionData() already supports vararg
	The sample clients need xfixes libs and cflags
	Fix memory leak when fetching the window regions
	Add the XColor header.
	Use the _NET_COLOR_PROFILES to store profiles in the root window.
	Use XColorRegion to communicate the window regions.
	Changed meaning of _NET_COLOR_MANAGEMENT. Now used to activate or deactivate parts of the region stack.
	client: Upload profile into the root window.
	Change pluginDrawWindow() to use the local region stack.
	Change pluginDrawWindowTexture() to use the local region stack.
	Remove debugging fprintf().
	Add preliminary XRandR output support.
	Fix handling of privates and allow objects to fail initialization.
	Listen to XRandR events and update outputs accordingly.
	Use the correct XRandR output property name.
	Don't crash if something goes wrong inside lcms.
	Let the client decide to which output device the colors should be converted.
	Add compiz plugin and net-color documentation
	Add comments to functions and code.
	Clarify the XColorProfile format.
	Xcolor: client-side API for managing profiles and regions.
	Reorganize files, clean up Makefile, swith the compiz plugin to the new Xcolor API
	Add a define that can be used to enable debugging output.
	Big reorganization of internal structures in the compiz plugin.
	Fix deletition of profiles.
	Replace UUID with MD5, as suggested by Kai-Uwe.
	Add some comments to the internal structures.
	client: Change target output at keypress.
	Replaced printf() with compLogMessage().
	Add function descriptions to the Xcolor header.
	Add a function for activating stack regions.
	Don't create the shader for each region separately.
	Increase the size of the color-managed region in the sample client.
	Add Xinerama screen detection to the sample client.

