gdt_start:
    dq 0
gdt_code32:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0xCF
    db 0x00
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
gdt_code16:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0x0F
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code32 - gdt_start
DATA_SEG equ gdt_data - gdt_start
CODE16_SEG equ gdt_code16 - gdt_start

%ifndef STACK_TOP
STACK_TOP equ 0x0009F000
%endif