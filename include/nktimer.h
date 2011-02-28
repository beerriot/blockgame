#ifndef __NKTIMER_H__
#define __NKTIMER_H__

#define F_CPU 14745600

void nktimer_init();
void nktimer_resume();
void nktimer_pause();
uint8_t nktimer_animate();
void nktimer_simple_delay(int clicks);

#endif
