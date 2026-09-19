[BITS 32]
section .text

extern isr_dispatch

%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0
    push dword %1
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push dword %1
    jmp isr_common
%endmacro

%assign i 0
%rep 32
    %if i = 8 || i = 10 || i = 11 || i = 12 || i = 13 || i = 14 || i = 17 || i = 21
        ISR_ERR i
    %else
        ISR_NOERR i
    %endif
    %assign i i+1
%endrep

%assign i 32
%rep 16
    ISR_NOERR i
    %assign i i+1
%endrep

isr_common:
    pusha
    push esp
    call isr_dispatch
    add esp, 4
    popa
    add esp, 8
    iret

section .data
global isr_handlers
isr_handlers:
%macro ISR_TBL_ENTRY 1
    dd isr%1
%endmacro
%assign i 0
%rep 48
    ISR_TBL_ENTRY i
    %assign i i+1
%endrep