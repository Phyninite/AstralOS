#ifndef TIMER_H
#define TIMER_H

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

void timer_init();
void timer_delay_ms(uint32_t ms);
void timer_enable_interrupt();
void timer_disable_interrupt();
void timer_handle_interrupt();

#endif


