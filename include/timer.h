#ifndef __TIMER_H__
#define __TIMER_H__

#define F_CPU 14745600

void boot_timer();
uint8_t animate();
void simple_delay(int clicks);

#endif
