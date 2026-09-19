; switch to 32 bit protected mode and jump to the kernel yay
goto_pm:
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEG:init_pm

[BITS 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; setup stack
    mov esp, STACK_TOP

    call KERNEL_PHYS

.hang:
    hlt
    jmp .hang