#include "vm_maps.h"
#include "cpu.h"
#include "crash_core.h"
#include "font.h"
#include "dtb.h"
#include "security.h"
#include "astral_sched.h"
#include "kmalloc.h"
#include "kprintf.h"
#include "fs.h"
#include "block_device.h"
#include "vfs.h"         
#include "../drivers/timer/timer.h"
#include "lib.h"

extern void _exception_vectors();

void dummy_task_func_a() {
    int counter = 0;
    while (1) {
        kprintf("task a running! counter: %d\n", counter);
        counter++;
        sched_yield();
    }
}

void dummy_task_func_b() {
    int counter = 0;
    while (1) {
        kprintf("task b running! counter: %d\n", counter);
        counter++;
        sched_yield();
    }
}

void kernel_main(uint64_t dtb_addr) {
    uart_init(); // initialize uart early

    check_and_halt_core();
    cpu_set_state(CPU_STATE_RUNNING);
    
    asm volatile("msr vbar_el1, %0" : : "r"((uint64_t)_exception_vectors));

    vm_init();
    cpu_enable_mmu(); 
    
    uint64_t fb_base = 0;
    uint32_t fb_width = 0;
    uint32_t fb_height = 0;
    uint32_t fb_pitch = 0;

    dtb_init(dtb_addr);

    // explicitly parse framebuffer info from dtb
    uint32_t len;
    const uint32_t* prop_val;

    prop_val = (const uint32_t*)dtb_get_property("/chosen", "framebuffer", &len);
    if (prop_val && len >= (4 * sizeof(uint32_t))) {
        fb_base = bswap32(prop_val[0]);
        fb_width = bswap32(prop_val[1]);
        fb_height = bswap32(prop_val[2]);
        fb_pitch = bswap32(prop_val[3]);
    } else {
        // if dtb info not found, panic or use serial only
        crash_core_panic("framebuffer info not found in dtb!\n");
    }

    // initialize kprintf with framebuffer parameters
    uint32_t* fb = (uint32_t*)fb_base;
    kprintf_init(fb, fb_width, fb_height, fb_pitch);

    // clear the framebuffer
    for (int i = 0; i < (fb_height * fb_pitch) / 4; i++) {
        fb[i] = 0x00000000;
    }
    
    kprintf("astral os\n");

    // initialize kmalloc with memory range obtained from dtb if available
    uint64_t mem_start, mem_size;
    if (dtb_get_memory_info(&mem_start, &mem_size) == 0) {
        kmalloc_init(mem_start, mem_size);
    } else {
        kmalloc_init(0x80000000, 256 * 1024 * 1024);
    }

    // set the active block device and initialize the filesystem layer
    set_active_block_device(BLOCK_DEVICE_TYPE_UFS);
    fs_init();

    // initialize the virtual file system and create the root directory
    if (vfs_init() != 0) {
        kprintf("kernel_main: vfs initialization failed!\n");
    } else {
        vfs_node_t *root_dir = vfs_get_root();
        if (!root_dir) {
            kprintf("kernel_main: could not get root vfs directory\n");
        } else {
            // create some basic directories under the root
            vfs_create_dir(root_dir, "bin");
            vfs_create_dir(root_dir, "etc");
            vfs_create_dir(root_dir, "dev");
            kprintf("vfs root directory initialized with 'bin', 'etc', and 'dev'\n");
        }
    }
    
    sched_init();
    sched_create_task(dummy_task_func_a, 4096);
    sched_create_task(dummy_task_func_b, 4096);

    timer_init();
    timer_enable_interrupt();
    cpu_enable_interrupts();

    sched_schedule();

    while (1) {
        cpu_set_state(CPU_STATE_HALTED);
    }
}

void exception_handler(uint64_t sp, uint32_t type) {
    crash_core_panic("unhandled exception type %d at sp 0x%llx\n", type, sp);
}
