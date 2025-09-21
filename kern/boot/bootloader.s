.section .text.boot

.globl _start
_start:
    mrs x1, mpidr_el1
    and x1, x1, #0xff
    cbz x1, master_core
    b halt

master_core:
    ldr x1, =_start
    mov sp, x1

    ldr x1, =__bss_start
    ldr w2, =__bss_size
    cbz w2, done_bss
clear_bss:
    str xzr, [x1], #8
    sub w2, w2, #1
    cbnz w2, clear_bss
done_bss:

    ldr x0, =_dtb_ptr
    ldr x0, [x0]
    bl kernel_main

halt:
    wfi
    b halt


