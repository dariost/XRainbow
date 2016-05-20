CC ?= gcc
LDFLAGS = -lX11 -lXxf86vm -lm
CFLAGS = -std=gnu99 -O2

all:
	$(CC) xrainbow.c $(CFLAGS) $(LDFLAGS) -o xrainbow
