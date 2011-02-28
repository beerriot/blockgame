#ifndef __NKLCD_H__
#define __NKLCD_H__

#define DISPLAY_CMD    0x08
#define DISPLAY_BLINK  0x01
#define DISPLAY_CURSOR 0x02
#define DISPLAY_ON     0x04

void nklcd_init();
void nklcd_start_blinking();
void nklcd_stop_blinking();
void nklcd_off();
void nklcd_on();

#endif
