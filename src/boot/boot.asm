; typeos boot sector: 16-bit real mode -> 32-bit protected mode.
; Loads the kernel (a flat binary) from floppy to KERNEL_PHYS (1 MiB) and
; jumps to it.  See comments in read_chunk for the bounce strategy.
[ORG 0x7C00]

KERNEL_PHYS equ 0x100000           ; final physical load address of the kernel
BOUNCE_SEG equ 0x8000              ; bounce buffer segment (phys 0x80000)
BOUNCE_PHYS equ 0x80000
CHUNK_SECTORS equ 64               ; 64 sectors = 32 KiB per trip

[BITS 16]
boot:
    ; set up our own stack (below the bounce buffer, safe RAM)
    mov ax, 0x7000
    mov ss, ax
    mov sp, 0x0000

    mov [BOOT_DRIVE], dl
    mov ax, 0x0003                 ; 80x25 color text mode
    int 0x10

    ; ---- load loop ----
    mov bp, NUM_KERNEL_SECTORS     ; sectors remaining
    mov dword [dst_off], KERNEL_PHYS
    mov byte [cur_cyl], 0
    mov byte [cur_head], 0
    mov byte [cur_sect], 2         ; kernel starts at sector 2

.copy_loop:
    test bp, bp
    jz .copy_done

    mov ax, bp
    cmp ax, CHUNK_SECTORS
    jbe .chunk_ok
    mov ax, CHUNK_SECTORS
.chunk_ok:
    mov word [chunk], ax

    mov ax, BOUNCE_SEG
    mov es, ax
    xor bx, bx
    mov cx, [chunk]
    call read_chunk                ; in: cx=count, es:bx=buf, cur_* state

    cli
    mov eax, BOUNCE_PHYS
    mov dword [pm_src], eax
    mov eax, [dst_off]
    mov dword [pm_dst], eax
    movzx eax, word [chunk]
    shl eax, 7                     ; sectors * 128 = dwords
    mov dword [pm_cnt], eax
    call pm_copy_once

    movzx eax, word [chunk]
    shl eax, 9
    add [dst_off], eax
    mov ax, [chunk]
    sub bp, ax
    jmp .copy_loop

.copy_done:
    call goto_pm

disk_error:
    mov bx, MSG_DISK_ERROR
    call print
    jmp $

; read_chunk: read CX sectors at (cur_cyl,cur_head,cur_sect) into es:bx,
; advancing the cur_* globals.  CF=0 on success.
read_chunk:
    test cx, cx
    jz .rdone
.rloop:
    ; sectors left in current track: 18 - cur_sect + 1
    mov al, [cur_sect]              ; sector (byte!)
    mov ah, 0
    mov si, ax
    mov ax, 19
    sub ax, si                     ; ax = 18 - cur_sect + 1 = sectors left in track
    cmp ax, cx                     ; take min(ax, cx)
    jbe .have
    mov ax, cx
.have:
    mov si, ax                     ; si = chunk to read now

    pusha
    mov ax, si
    mov ah, 0x02
    mov ch, [cur_cyl]
    mov dh, [cur_head]
    mov cl, [cur_sect]
    mov dl, [BOOT_DRIVE]
    int 0x13
    popa
    jc disk_error

    ; es:bx += si*512
    movzx eax, bx
    movzx ebx, si
    shl ebx, 9
    add eax, ebx
    mov bx, ax
    shr eax, 16
    jz .nopg
    mov ax, es
    add ax, 0x1000
    mov es, ax
.nopg:

    ; CHS advance by si sectors
    mov ax, si
    add [cur_sect], al
    mov al, [cur_sect]
    cmp al, 18
    jbe .nwrp
    sub al, 18
    mov [cur_sect], al
    xor byte [cur_head], 1
    cmp byte [cur_head], 0
    jne .nwrp
    inc byte [cur_cyl]
.nwrp:

    sub cx, si
    jnz .rloop
.rdone:
    ret

%include "print.asm"
%include "pm_copy.asm"
%include "protected_mode.asm"
%include "gdt.asm"

; ---- data ----
BOOT_DRIVE db 0
cur_cyl    db 0
cur_head   db 0
cur_sect   db 2
chunk      dw 0
dst_off    dd KERNEL_PHYS

MSG_DISK_ERROR:  db "disk error", 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55