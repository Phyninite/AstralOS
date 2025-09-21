#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdarg.h>

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

void kprintf_init(uint32_t* fb, uint32_t width, uint32_t height, uint32_t pitch);
void kprintf(const char* fmt, ...);
void kprintf_varg(const char* fmt, va_list args);

#endif // KPRINTF_H


