GCCFLAGS=-g -Os -Wall -mmcu=atmega168 -Iinclude -I../libnerdkits
LINKFLAGS=
AVRDUDEFLAGS=-c avr109 -p m168 -b 115200 -P /dev/cu.PL2303-0000101D
LINKOBJECTS=../libnerdkits/delay.o ../libnerdkits/lcd.o

all:	blockgame-upload

blockgame.hex: blockgame.c src/nkbuttons.c src/nkeeprom.c src/nkrand.c src/nklcd.c src/nktimer.c src/bghighscore.c
	make -C ../libnerdkits
	avr-gcc ${GCCFLAGS} ${LINKFLAGS} -o blockgame.o \
		blockgame.c src/nkbuttons.c src/nkeeprom.c src/nkrand.c \
		src/nklcd.c src/nktimer.c src/bghighscore.c ${LINKOBJECTS}
	avr-objcopy -j .text -O ihex blockgame.o blockgame.hex

blockgame.ass:	blockgame.hex
	avr-objdump -S -d blockgame.o > blockgame.ass

blockgame-upload:	blockgame.hex
	avrdude ${AVRDUDEFLAGS} -U flash:w:blockgame.hex:a
