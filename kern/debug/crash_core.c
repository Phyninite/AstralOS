#include "crash_core.h"
#include "cpu.h"
#include "kprintf.h"
#include "lib.h"

#define MAX_CORES 8

void check_and_halt_core() {
    uint64_t core_id = cpu_get_core_id();
    if (core_id != 0) {
        cpu_set_state(CPU_STATE_HALTED);
    }
}

void crash_core(uint64_t core_id) {
    if (core_id >= MAX_CORES) {
        return;
    }
    if (cpu_get_core_id() == core_id) {
        cpu_set_state(CPU_STATE_HALTED);
    }
}

void crash_core_panic(const char* fmt, ...) {
    kprintf("kernel panic: ");
    va_list args;
    va_start(args, fmt);
    kprintf_varg(fmt, args);
    va_end(args);
    kprintf("\n");
    cpu_set_state(CPU_STATE_HALTED);
    while (1);
}


