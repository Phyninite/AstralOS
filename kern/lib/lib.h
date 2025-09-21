#ifndef LIB_H
#define LIB_H

#include <stdarg.h>

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

void* memset(void* s, int c, uint64_t n);
void* memcpy(void* dest, const void* src, uint64_t n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, uint64_t n);
char* strncpy(char* dest, const char* src, uint64_t n);
char* strchr(const char* s, int c);
uint64_t strlen(const char* s);
int snprintf(char* str, uint64_t size, const char* format, ...);
int vsnprintf(char* str, uint64_t size, const char* format, va_list ap);
void uart_putc(char c);
void uart_init();

#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & ~((align) - 1))

#endif // LIB_H


