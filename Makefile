
CFLAGS = -Wall -Iinclude -std=c99 -g -ggdb -pedantic


# The X11 client-side library
libXcolor.so: src/Xcolor/Xcolor.c
	gcc $(CFLAGS) -fPIC -shared -o $@ $< -lX11

# The compiz plugin
libcolor.so: src/compiz/color.c
	gcc $(CFLAGS) -fPIC -shared -o $@ $< `pkg-config --cflags --libs compiz` -llcms

client: src/client.c
	gcc $(CFLAGS) -o $@ $< `pkg-config --cflags --libs cairo x11 xfixes xrandr xinerama` -L. -lXcolor

all: libcolor.so libXcolor.so client

install: libcolor.so
	mkdir -p $(HOME)/.compiz/plugins
	install libcolor.so $(HOME)/.compiz/plugins/libcolor.so

clean:
	rm -f libcolor.so libXcolor.so client

