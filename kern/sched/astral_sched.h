#ifndef ASTRAL_SCHED_H
#define ASTRAL_SCHED_H

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

#define MAX_TASKS 8

typedef struct {
    uint64_t sp;
    uint64_t lr;
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t fp;
} cpu_context_t;

typedef struct {
    uint32_t id;
    cpu_context_t context;
    uint32_t state;
    uint64_t stack_base;
    uint64_t stack_size;
    uint64_t ttbr0_el1; // page table base register for this task
} tcb_t;

typedef struct {
    volatile uint32_t lock;
} spinlock_t;

void spinlock_init(spinlock_t* lock);
void spinlock_acquire(spinlock_t* lock);
void spinlock_release(spinlock_t* lock);

void sched_init();
void sched_add_task(tcb_t* task);
void sched_schedule();
void sched_yield();
void sched_create_task(void (*func)(), uint64_t stack_size);

#endif


