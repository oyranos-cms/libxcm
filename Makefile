
CFLAGS = -Wall -Iinclude -std=c99 -g -ggdb `pkg-config --cflags compiz`
LIBS   = `pkg-config --libs compiz` -llcms

OBJS   = src/color.o

%.o: %.c
	gcc $(CFLAGS) -fPIC -c -o $@ $<

libcolor.so: $(OBJS)
	gcc $(LIBS) -fPIC -shared -o $@ $<

all: libcolor.so
	gcc $(CFLAGS) -o client src/client.c -lX11 `pkg-config --cflags --libs cairo xfixes` 

install:
	mkdir -p $(HOME)/.compiz/plugins
	install libcolor.so $(HOME)/.compiz/plugins/libcolor.so

clean:
	rm -f $(OBJS) libcolor.so

