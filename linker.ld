ENTRY(_start)

SECTIONS
{
    . = 0x100000;

    .text : {
        KEEP(*(.entry))
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)
    }

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }

    . = ALIGN(16);
    PROVIDE(__kernel_end = .);

    /DISCARD/ : {
        *(.eh_frame)
        *(.note*)
        *(.comment)
    }
}