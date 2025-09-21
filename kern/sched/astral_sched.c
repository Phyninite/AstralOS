#include "astral_sched.h"
#include "cpu.h"
#include "kprintf.h"
#include "lib.h"
#include "../memory/kmalloc.h"
#include "../memory/vm_maps.h"

static tcb_t* current_task = 0;
static tcb_t* task_list[MAX_TASKS];
static int num_tasks = 0;
static int current_task_index = -1;
static spinlock_t sched_lock;

extern void context_switch(cpu_context_t* old_context, cpu_context_t* new_context);

void spinlock_init(spinlock_t* lock) {
    lock->lock = 0;
}

void spinlock_acquire(spinlock_t* lock) {
    uint32_t temp;
    asm volatile(
        "1: ldaxr %w0, [%1]\n"
        "   cbnz %w0, 1b\n"
        "   stxr %w0, %w2, [%1]\n"
        "   cbnz %w0, 1b\n"
        : "=&r"(temp)
        : "r"(&lock->lock), "r"(1)
        : "memory"
    );
}

void spinlock_release(spinlock_t* lock) {
    asm volatile(
        "stlr %w1, [%0]\n"
        : : "r"(&lock->lock), "r"(0)
        : "memory"
    );
}

void sched_init() {
    memset(task_list, 0, sizeof(task_list));
    num_tasks = 0;
    current_task_index = -1;
    spinlock_init(&sched_lock);
}

void sched_add_task(tcb_t* task) {
    spinlock_acquire(&sched_lock);
    if (num_tasks >= MAX_TASKS) {
        spinlock_release(&sched_lock);
        return;
    }
    task_list[num_tasks] = task;
    num_tasks++;
    spinlock_release(&sched_lock);
}

void sched_schedule() {
    spinlock_acquire(&sched_lock);
    if (num_tasks == 0) {
        spinlock_release(&sched_lock);
        cpu_set_state(CPU_STATE_HALTED);
        return;
    }

    if (current_task == 0) {
        current_task_index = 0;
        current_task = task_list[current_task_index];
        asm volatile("msr ttbr0_el1, %0" : : "r"(current_task->ttbr0_el1));
        spinlock_release(&sched_lock);
        asm volatile(
            "mov sp, %0\n"
            "mov x30, %1\n"
            "br x30"
            : : "r"(current_task->context.sp), "r"(current_task->context.lr)
        );
    } else {
        spinlock_release(&sched_lock);
        sched_yield();
    }
}

void sched_yield() {
    spinlock_acquire(&sched_lock);
    if (num_tasks <= 1) {
        spinlock_release(&sched_lock);
        return;
    }

    cpu_context_t* old_context = &current_task->context;

    current_task_index = (current_task_index + 1) % num_tasks;
    current_task = task_list[current_task_index];

    cpu_context_t* new_context = &current_task->context;
    asm volatile("msr ttbr0_el1, %0" : : "r"(current_task->ttbr0_el1));
    spinlock_release(&sched_lock);
    context_switch(old_context, new_context);
}

void sched_create_task(void (*func)(), uint64_t stack_size) {
    tcb_t* new_task = (tcb_t*)kmalloc(sizeof(tcb_t));
    if (!new_task) {
        return;
    }
    new_task->id = num_tasks;
    new_task->stack_base = (uint64_t)kmalloc(stack_size);
    if (!new_task->stack_base) {
        kfree(new_task);
        return;
    }
    new_task->stack_size = stack_size;

    new_task->context.sp = new_task->stack_base + stack_size - 16;
    new_task->context.lr = (uint64_t)func;
    new_task->context.fp = new_task->stack_base + stack_size - 16;
    new_task->ttbr0_el1 = vm_create_task_pagetable();
    if (!new_task->ttbr0_el1) {
        kfree((void*)new_task->stack_base);
        kfree(new_task);
        return;
    }

    sched_add_task(new_task);
}


