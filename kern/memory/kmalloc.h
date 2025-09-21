#ifndef KMALLOC_H
#define KMALLOC_H

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

void kmalloc_init(uint64_t heap_start, uint64_t heap_size);
void* kmalloc(uint64_t size);
void kfree(void* ptr);

#endif // KMALLOC_H


