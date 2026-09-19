;; actual entry
[BITS 32]
section .entry

global _start
extern kmain

; STACK_TOP is injected by the Makefile/-DSTACK_TOP=... from src/kernel/kernel.h
%ifndef STACK_TOP
STACK_TOP equ 0x0009F000
%endif

_start:
    cli
    mov esp, STACK_TOP
    call kmain

.hang:
    hlt
    jmp .hang