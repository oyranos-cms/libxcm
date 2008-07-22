
CFLAGS = -Wall -Iinclude -std=c99 -g -ggdb `pkg-config --cflags compiz`
LIBS   = `pkg-config --libs compiz` -llcms -luuid

OBJS   = src/color.o

%.o: %.c
	gcc $(CFLAGS) -fPIC -c -o $@ $<

libXcolor.so: src/Xcolor/Xcolor.o
	gcc -I src/Xcolor -luuid -fPIC -shared -o libXcolor.so $<

libcolor.so: $(OBJS)
	gcc $(LIBS) -fPIC -shared -o $@ $<

all: libcolor.so libXcolor.so
	gcc $(CFLAGS) -o client src/client.c -lX11 -luuid `pkg-config --cflags --libs cairo xfixes`  -L. -lXcolor

install:
	mkdir -p $(HOME)/.compiz/plugins
	install libcolor.so $(HOME)/.compiz/plugins/libcolor.so

clean:
	rm -f $(OBJS) libcolor.so libXcolor.so

