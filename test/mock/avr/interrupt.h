/* interrupt.h: Mock definitions for testing */
#ifndef __INTERRUPT_H_
#define __INTERRUPT_H_

#define ISR(x) void x()

void cli();
void sei();

#endif
