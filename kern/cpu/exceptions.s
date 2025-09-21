.global _exception_vectors

.extern exception_handler
.extern timer_handle_interrupt

_exception_vectors:
    b el1_sync
    b el1_irq
    b el1_fiq
    b el1_serror

    b el1_sync
    b el1_irq
    b el1_fiq
    b el1_serror

    b el0_sync
    b el0_irq
    b el0_fiq
    b el0_serror

    b el0_sync
    b el0_irq
    b el0_fiq
    b el0_serror

.macro save_context
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp x6, x7, [sp, #-16]!
    stp x8, x9, [sp, #-16]!
    stp x10, x11, [sp, #-16]!
    stp x12, x13, [sp, #-16]!
    stp x14, x15, [sp, #-16]!
    stp x16, x17, [sp, #-16]!
    stp x18, x19, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x24, x25, [sp, #-16]!
    stp x26, x27, [sp, #-16]!
    stp x28, x29, [sp, #-16]!
    mrs x20, elr_el1
    mrs x21, spsr_el1
    stp x30, x20, [sp, #-16]!
    stp x21, xzr, [sp, #-16]!
.endm

.macro restore_context
    ldp x21, xzr, [sp], #16
    ldp x30, x20, [sp], #16
    msr elr_el1, x20
    msr spsr_el1, x21
    ldp x28, x29, [sp], #16
    ldp x26, x27, [sp, #-16]!
    ldp x24, x25, [sp, #-16]!
    ldp x22, x23, [sp, #-16]!
    ldp x20, x21, [sp, #-16]!
    ldp x18, x19, [sp, #-16]!
    ldp x16, x17, [sp, #-16]!
    ldp x14, x15, [sp, #-16]!
    ldp x12, x13, [sp, #-16]!
    ldp x10, x11, [sp, #-16]!
    ldp x8, x9, [sp, #-16]!
    ldp x6, x7, [sp, #-16]!
    ldp x4, x5, [sp, #-16]!
    ldp x2, x3, [sp, #-16]!
    ldp x0, x1, [sp], #16
.endm

el1_sync:
    save_context
    mov x0, sp
    mov x1, #0
    bl exception_handler
    restore_context
    eret

el1_irq:
    save_context
    mov x0, sp
    mov x1, #1
    bl timer_handle_interrupt
    restore_context
    eret

el1_fiq:
    save_context
    mov x0, sp
    mov x1, #2
    bl exception_handler
    restore_context
    eret

el1_serror:
    save_context
    mov x0, sp
    mov x1, #3
    bl exception_handler
    restore_context
    eret

el0_sync:
    save_context
    mov x0, sp
    mov x1, #4
    bl exception_handler
    restore_context
    eret

el0_irq:
    save_context
    mov x0, sp
    mov x1, #5
    bl exception_handler
    restore_context
    eret

el0_fiq:
    save_context
    mov x0, sp
    mov x1, #6
    bl exception_handler
    restore_context
    eret

el0_serror:
    save_context
    mov x0, sp
    mov x1, #7
    bl exception_handler
    restore_context
    eret


