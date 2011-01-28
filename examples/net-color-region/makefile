DEPS := $(shell PKG_CONFIG_PATH=/opt/local/lib64/pkgconfig pkg-config --cflags --libs x11 xfixes xcm oyranos)
DEBUG = -Wall -pedantic -g 
CC = gcc

all:
	$(CC) $(DEBUG) $(INCL) netColorRegion2.c $(DEPS) $(LIBS) -o netColorRegion2
