VPATH=../src:../include:mock:mock/avr
CC=gcc
MOCK=mock
CFLAGS=-g -Os -Wall -I../include -I$(MOCK)
NKOBJECTS=lcd.o
AVROBJECTS=sleep.o interrupt.o
OBJECTS=bggame.o bgmenu.o bghighscore.o \
	nktimer.o nklcd.o nkrand.o nkeeprom.o nkbuttons.o nksleep.o

.PHONY: clean test

test: bgtest
	./bgtest

bgtest: $(OBJECTS) $(NKOBJECTS) $(AVROBJECTS) bgtest.c
	$(CC) $(CFLAGS) $^ -o bgtest

clean:
	-rm *.o *.d bgtest

-include $(OBJECTS:%.o=%.d)

deps: $(OBJECTS:%.o=%.d) $(NKOBJECTS:%.o=%.d) $(AVROBJECTS:%.0=%.d)

%.d: %.c
	$(CC) $(CFLAGS) -MM $< > $@
