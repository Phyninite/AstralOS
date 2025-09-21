#include "cpu.h"

#define MAX_CORES 8

static volatile cpu_state_t cpu_states[MAX_CORES];

void cpu_enable_interrupts() {
    asm volatile("msr daifclr, #2");
}

void cpu_disable_interrupts() {
    asm volatile("msr daifset, #2");
}

void cpu_wfi() {
    asm volatile("wfi");
}

void cpu_wfe() {
    asm volatile("wfe");
}

void cpu_sev() {
    asm volatile("sev");
}

void cpu_sevl() {
    asm volatile("sevl");
}

uint64_t cpu_get_core_id() {
    uint64_t mpidr;
    asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    return (mpidr & 0xff) | ((mpidr >> 8) & 0xff00);
}

void cpu_set_state(cpu_state_t state) {
    uint64_t core_id = cpu_get_core_id();
    if (core_id >= MAX_CORES) return;
    
    cpu_states[core_id] = state;
    
    switch (state) {
        case CPU_STATE_IDLE:
            while (cpu_states[core_id] == CPU_STATE_IDLE) {
                cpu_wfi();
            }
            break;
            
        case CPU_STATE_HALTED:
            cpu_disable_interrupts();
            while (1) {
                cpu_wfi();
            }
            break;
            
        case CPU_STATE_RUNNING:
        default:
            break;
    }
}

cpu_state_t cpu_get_state() {
    uint64_t core_id = cpu_get_core_id();
    return (core_id < MAX_CORES) ? cpu_states[core_id] : CPU_STATE_RUNNING;
}

uint64_t cpu_get_system_timer_count() {
    uint64_t cntpct_el0;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0));
    return cntpct_el0;
}


