
CFLAGS = -Wall -Iinclude -std=c99 -g -ggdb


# The X11 client-side library
libXcolor.so: src/Xcolor/Xcolor.c
	gcc $(CFLAGS) -fPIC -shared -o $@ $<

# The compiz plugin
libcolor.so: src/compiz/color.c
	gcc $(CFLAGS) -fPIC -shared -o $@ $< `pkg-config --cflags --libs compiz` -llcms

client: src/client.c
	gcc $(CFLAGS) -o $@ $< `pkg-config --cflags --libs cairo xfixes x11` -L. -lXcolor -luuid

all: libcolor.so libXcolor.so client

install: libcolor.so
	mkdir -p $(HOME)/.compiz/plugins
	install libcolor.so $(HOME)/.compiz/plugins/libcolor.so

clean:
	rm -f libcolor.so libXcolor.so client

