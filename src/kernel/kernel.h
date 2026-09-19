#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define KERNEL_BASE 0x100000UL
#define STACK_TOP 0x800000UL
#define HEAP_END 0x400000UL

// VGA text mode: 80x25 color
#define VGA_W 80
#define VGA_H 25

struct isr_regs {
	uint32_t edi, esi, ebp, esp_orig, ebx, edx, ecx, eax;
	uint32_t intno, err, eip, cs, eflags;
};

void serial_init(void);
void serial_putc(char c);
void serial_puts(const char* s);

// VGA text console
void vga_clear(void);
void vga_set_attr(uint8_t attr);
uint8_t vga_get_attr(void);
void vga_putc(char c);
void vga_write(const char* s);
void vga_putc_attr(char c, uint8_t attr);
void vga_write_attr(const char* s, uint8_t attr);
void vga_set_cursor(int x, int y);
void vga_get_cursor(int* x, int* y);
void vga_delete_char(void);
uint8_t vga_color_attr(const char* name, uint8_t fallback);

// keyboard
void kbd_irq(void);		  // called from the IRQ1 path
char* kbd_readline(void); // blocking. waits for a full line

// interrupts
void idt_init(void);
void isr_dispatch(struct isr_regs* r);

// kernel
void kmain(void);
void panic(const char* msg) __attribute__((noreturn));
void kprintf(const char* fmt, ...);
void kdebug(const char* msg);