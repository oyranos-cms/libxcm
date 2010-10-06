edid-parse:	edid_parse.c
	cc -Wall -g edid_parse.c -o edid-parse `pkg-config --cflags --libs xcm`\
	-lm
check:	edid-parse
	oyranos-monitor -f edid -o edid.bin
	./edid-parse edid.bin
	if [ `oyranos-monitor -l | wc -l` -gt 1 ]; then \
	  oyranos-monitor -x 2000 -f edid -o edid2.bin; \
	  ./edid-parse edid2.bin; \
	fi
