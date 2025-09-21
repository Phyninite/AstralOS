#include "timer.h"
#include "../../cpu/cpu.h"
#include "../../sched/astral_sched.h"

#define CNTFRQ_EL0 (volatile uint64_t*)0x40000000
#define CNTP_CTL_EL0 (volatile uint32_t*)0x40000008
#define CNTP_TVAL_EL0 (volatile uint32_t*)0x4000000C
#define CNTP_CVAL_EL0 (volatile uint64_t*)0x40000010

static uint64_t timer_frequency = 0;

void timer_init() {
    timer_frequency = *CNTFRQ_EL0;
}

void timer_enable_interrupt() {
    *CNTP_TVAL_EL0 = timer_frequency / 100; // 10ms interrupt
    *CNTP_CTL_EL0 = 1; // enable timer, enable interrupt
}

void timer_disable_interrupt() {
    *CNTP_CTL_EL0 = 0; // disable timer, disable interrupt
}

void timer_handle_interrupt() {
    *CNTP_TVAL_EL0 = timer_frequency / 100; // reset timer for next interrupt
    sched_yield();
}


