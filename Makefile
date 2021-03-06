VPATH=src:include
CC=avr-gcc
LIBNERDKITS=../libnerdkits
CFLAGS=-g -Os -Wall -mmcu=atmega168 -Iinclude -I$(LIBNERDKITS)
AVRDUDEFLAGS=-c avr109 -p m168 -b 115200 -P /dev/cu.PL2303-0000101D
NKOBJECTS=$(LIBNERDKITS)/delay.o $(LIBNERDKITS)/lcd.o
OBJECTS=bggame.o bgmenu.o bghighscore.o \
	nktimer.o nklcd.o nkrand.o nkeeprom.o nkbuttons.o nksleep.o

all: blockgame.hex

upload: all
	avrdude $(AVRDUDEFLAGS) -U flash:w:blockgame.hex:a

blockgame.hex: blockgame
	avr-objcopy -j .text -O ihex blockgame blockgame.hex

blockgame: $(OBJECTS) blockgame.c
	$(CC) $(CFLAGS) $^ -o blockgame $(NKOBJECTS)

blockgame.ass:	blockgame
	avr-objdump -S -d blockgame > blockgame.ass

.PHONY: clean test
clean:
	-rm *.o *.d blockgame blockgame.hex blockgame.ass
	$(MAKE) -C test clean

-include $(OBJECTS:%.o=%.d)

deps: $(OBJECTS:%.o=%.d)

%.d: %.c
	$(CC) $(CFLAGS) -MM $< > $@

test:
	$(MAKE) -C test test
