[BITS 16]

pm_copy_once:
    mov word [rm_ss], ss
    mov word [rm_sp], sp

    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:.in_pm

.in_pm:
[BITS 32]
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; copy
    mov esi, [pm_src]
    mov edi, [pm_dst]
    mov ecx, [pm_cnt]
    rep movsd

    ; far jump to the 16bit code segment (still protected mode, but the CPU now decodes 16bit because the descriptor has D=0)
    jmp CODE16_SEG:.in_16

.in_16:
[BITS 16]
    ; now in 16 bit protected mode ok
    mov eax, cr0
    and eax, 0xFFFFFFFE
    mov cr0, eax
    jmp 0:.back_rm

.back_rm:
[BITS 16]
    ; restore realmode stack & segments
    mov ax, [rm_ss]
    mov ss, ax
    mov sp, [rm_sp]
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    sti
    ret

; state block (set by boot.asm before each call)
pm_src dd 0
pm_dst dd 0
pm_cnt dd 0
rm_ss  dw 0
rm_sp  dw 0