all:	client libcolor.so
client:	client.c
	gcc -Wall -g -I../../include/ -o client client.c `pkg-config --cflags --libs cairo x11 xfixes xrandr xinerama oyranos` -L../../ -lXcm
libcolor.so: color.c
	gcc -Wall -g -shared -fPIC -I../../include/ -o libcolor.so color.c `pkg-config --cflags --libs cairo x11 xfixes xrandr xinerama oyranos compiz lcms` -L../../ -lXcm

check:	client
	./client scanner.icc
