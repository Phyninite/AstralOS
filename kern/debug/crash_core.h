#ifndef CRASH_CORE_H
#define CRASH_CORE_H

#include <stdarg.h>

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

void check_and_halt_core();
void crash_core(uint64_t core_id);
void crash_core_panic(const char* fmt, ...);

#endif // CRASH_CORE_H


