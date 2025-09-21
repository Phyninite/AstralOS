#include "kmalloc.h"
#include "kprintf.h"
#include "lib.h"

typedef struct block_header {
    uint64_t size;
    struct block_header* next;
    int free;
} block_header_t;

static block_header_t* heap_start_ptr = 0;
static uint64_t total_heap_size = 0;

void kmalloc_init(uint64_t heap_start, uint64_t heap_size) {
    heap_start_ptr = (block_header_t*)heap_start;
    total_heap_size = heap_size;

    heap_start_ptr->size = heap_size - sizeof(block_header_t);
    heap_start_ptr->next = 0;
    heap_start_ptr->free = 1;
}

void* kmalloc(uint64_t size) {
    if (size == 0) return 0;

    size = (size + 7) & ~7;

    block_header_t* current = heap_start_ptr;
    while (current) {
        if (current->free && current->size >= size) {
            if (current->size > size + sizeof(block_header_t)) {
                block_header_t* new_block = (block_header_t*)((uint64_t)current + sizeof(block_header_t) + size);
                new_block->size = current->size - size - sizeof(block_header_t);
                new_block->next = current->next;
                new_block->free = 1;
                current->next = new_block;
                current->size = size;
            }
            current->free = 0;
            return (void*)((uint64_t)current + sizeof(block_header_t));
        }
        current = current->next;
    }
    return 0;
}

void kfree(void* ptr) {
    if (ptr == 0) return;

    block_header_t* block = (block_header_t*)((uint64_t)ptr - sizeof(block_header_t));
    block->free = 1;

    block_header_t* current = heap_start_ptr;
    while (current) {
        if (current->free && current->next && current->next->free && (uint64_t)current + sizeof(block_header_t) + current->size == (uint64_t)current->next) {
            current->size += sizeof(block_header_t) + current->next->size;
            current->next = current->next->next;
        }
        current = current->next;
    }
}


