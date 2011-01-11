# Makefile by Yno, all rights freeds :D

EXEC=scetest
OPTI=-g
PKGS=sdl sceinterface

CC ?= gcc
CFLAGS = -Wall $(OPTI) `pkg-config $(PKGS) --cflags`
LDFLAGS = `pkg-config $(PKGS) --libs`

all: $(EXEC)

$(EXEC): main.o
	$(CC) $(LDFLAGS) $^ -o $@

main.o: main.c Makefile
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o

mrproper: clean
	rm -f $(EXEC)

.PHONY: all clean mrproper
