ENTRY(_start)

SECTIONS
{
    . = 0x80000;

    .text : {
        KEEP(*(.text.boot))
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        __bss_start = .;
        *(.bss)
        __bss_end = .;
    }

    __bss_size = __bss_end - __bss_start;

    . = ALIGN(0x1000);
    .stack : {
        __stack_start = .;
        . += 0x4000; /* 16KB stack */
        __stack_end = .;
    }

    .dtb : ALIGN(0x1000) {
        _dtb_ptr = .;
        . += 0x1000; /* reserve space for dtb pointer */
    }

    /DISCARD/ : {
        *(.comment)
        *(.ARM.attributes)
    }
}


