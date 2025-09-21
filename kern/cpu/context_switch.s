.global context_switch

context_switch:
    // save current context (old_context)
    // x0 = old_context, x1 = new_context
    stp x0, x1, [x0, #0]
    stp x2, x3, [x0, #16]
    stp x4, x5, [x0, #32]
    stp x6, x7, [x0, #48]
    stp x8, x9, [x0, #64]
    stp x10, x11, [x0, #80]
    stp x12, x13, [x0, #96]
    stp x14, x15, [x0, #112]
    stp x16, x17, [x0, #128]
    stp x18, x19, [x0, #144]
    stp x20, x21, [x0, #160]
    stp x22, x23, [x0, #176]
    stp x24, x25, [x0, #192]
    stp x26, x27, [x0, #208]
    stp x28, x29, [x0, #224]
    str x30, [x0, #240]
    mrs x10, sp_el0
    str x10, [x0, #248]
    mrs x10, elr_el1
    str x10, [x0, #256]
    mrs x10, spsr_el1
    str x10, [x0, #264]

    // restore new context (new_context)
    ldp x0, x1, [x1, #0]
    ldp x2, x3, [x1, #16]
    ldp x4, x5, [x1, #32]
    ldp x6, x7, [x1, #48]
    ldp x8, x9, [x1, #64]
    ldp x10, x11, [x1, #80]
    ldp x12, x13, [x1, #96]
    ldp x14, x15, [x1, #112]
    ldp x16, x17, [x1, #128]
    ldp x18, x19, [x1, #144]
    ldp x20, x21, [x1, #160]
    ldp x22, x23, [x1, #176]
    ldp x24, x25, [x1, #192]
    ldp x26, x27, [x1, #208]
    ldp x28, x29, [x1, #224]
    ldr x30, [x1, #240]
    ldr x10, [x1, #248]
    msr sp_el0, x10
    ldr x10, [x1, #256]
    msr elr_el1, x10
    ldr x10, [x1, #264]
    msr spsr_el1, x10

    ret


