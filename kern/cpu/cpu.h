#ifndef CPU_H
#define CPU_H

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

typedef enum {
    CPU_STATE_IDLE,
    CPU_STATE_RUNNING,
    CPU_STATE_HALTED
} cpu_state_t;

void cpu_enable_interrupts();
void cpu_disable_interrupts();
void cpu_wfi();
void cpu_wfe();
void cpu_sev();
void cpu_sevl();
uint64_t cpu_get_core_id();
void cpu_set_state(cpu_state_t state);
cpu_state_t cpu_get_state();
uint64_t cpu_get_system_timer_count();
void cpu_enable_mmu();

#endif


