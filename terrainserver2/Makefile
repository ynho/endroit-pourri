# Makefile by Yno, all rights freeds :D

EXEC=scetest
OPTI=-g
PKGS=sdl sceinterface gl

CC ?= gcc
CFLAGS = -Wall $(OPTI) `pkg-config $(PKGS) --cflags`
LDFLAGS = `pkg-config $(PKGS) --libs`

all: $(EXEC)

$(EXEC): main.o perlin.o
	$(CC) $(LDFLAGS) $^ -o $@

main.o: main.c Makefile
	$(CC) $(CFLAGS) -c $<

perlin.o: perlin.c Makefile perlin.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o

mrproper: clean
	rm -f $(EXEC)

.PHONY: all clean mrproper
