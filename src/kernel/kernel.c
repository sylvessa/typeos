// typeos kernel: the thin C layer under the TypeScript shell.

// Everything here is just enough to get the QuickJS runtime running:
// serial, VGA text output, PS/2 keyboard, PIC/IDT, and a heap.
// All "OS logic" (shell, banner, commands) lives in src/ts.

#include "kernel.h"
#include "klib.h"
#include <string.h>
#include <stdio.h>

// x86 port i/o

static inline uint8_t inb(uint16_t port) {
	uint8_t v;
	__asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
	return v;
}

static inline void outb(uint16_t port, uint8_t val) { __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port)); }

static inline void io_wait(void) { outb(0x80, 0); }

// serial (COM1)

static int serial_up;

void serial_init(void) {
	outb(0x3F8 + 1, 0x00); // disable interrupts
	outb(0x3F8 + 3, 0x80); // DLAB on
	outb(0x3F8 + 0, 0x01); // divisor low -> 115200 baud
	outb(0x3F8 + 1, 0x00); // divisor high
	outb(0x3F8 + 3, 0x03); // 8N1, DLAB off
	outb(0x3F8 + 2, 0xC7); // FIFO on, clear, 14 byte threshold
	outb(0x3F8 + 4, 0x0B); // DTR + RTS
	serial_up = 1;
}

void serial_putc(char c) {
	if (!serial_up)
		return;
	while (!(inb(0x3F8 + 5) & 0x20))
		;
	outb(0x3F8, (uint8_t)c);
}

void serial_puts(const char* s) {
	if (!serial_up)
		return;
	while (*s)
		serial_putc(*s++);
}

// VGA text console

#define VGA_MEM ((volatile uint16_t*)0xB8000)

static int cur_x, cur_y;
static uint8_t cur_attr = 0x07; // light gray on black

static void vga_scroll(void) {
	for (int row = 0; row < VGA_H - 1; row++)
		memcpy((void*)(VGA_MEM + row * VGA_W), (const void*)(VGA_MEM + (row + 1) * VGA_W), VGA_W * 2);
	volatile uint16_t* last = VGA_MEM + (VGA_H - 1) * VGA_W;
	for (int x = 0; x < VGA_W; x++)
		last[x] = (uint16_t)(0x0700 | ' ');
	cur_y = VGA_H - 1;
}

static void vga_update_cursor(void) {
	uint16_t pos = (uint16_t)(cur_y * VGA_W + cur_x);
	outb(0x3D4, 14);
	outb(0x3D5, (uint8_t)(pos >> 8));
	outb(0x3D4, 15);
	outb(0x3D5, (uint8_t)pos);
}

void vga_clear(void) {
	for (int i = 0; i < VGA_W * VGA_H; i++)
		VGA_MEM[i] = (uint16_t)(0x0700 | ' ');
	cur_x = 0;
	cur_y = 0;
	cur_attr = 0x07;
	vga_update_cursor();
	serial_puts("\x1b[2J"); // clear for terminal emulators reading the log
	serial_puts("\x1b[H");
}

void vga_set_attr(uint8_t attr) { cur_attr = attr; }
uint8_t vga_get_attr(void) { return cur_attr; }

void vga_putc_attr(char c, uint8_t attr) {
	if (c == '\n') {
		cur_x = 0;
		cur_y++;
		if (cur_y >= VGA_H)
			vga_scroll();
	} else if (c == '\r') {
		cur_x = 0;
	} else if (c == '\b') {
		if (cur_x > 0)
			cur_x--;
	} else if (c == '\t') {
		do {
			cur_x++;
			if (cur_x >= VGA_W) {
				cur_x = 0;
				cur_y++;
				if (cur_y >= VGA_H)
					vga_scroll();
			}
		} while (cur_x % 8 != 0);
	} else {
		if (cur_x >= VGA_W) {
			cur_x = 0;
			cur_y++;
			if (cur_y >= VGA_H)
				vga_scroll();
		}
		VGA_MEM[cur_y * VGA_W + cur_x] = (uint16_t)(((uint16_t)attr << 8) | (uint8_t)c);
		cur_x++;
		if (cur_x >= VGA_W) {
			cur_x = 0;
			cur_y++;
			if (cur_y >= VGA_H)
				vga_scroll();
		}
	}
	vga_update_cursor();
	serial_putc(c); // lol
}

void vga_putc(char c) { vga_putc_attr(c, cur_attr); }
void vga_write_attr(const char* s, uint8_t attr) {
	while (*s)
		vga_putc_attr(*s++, attr);
}
void vga_write(const char* s) { vga_write_attr(s, cur_attr); }

void vga_set_cursor(int x, int y) {
	cur_x = x < 0 ? 0 : (x >= VGA_W ? VGA_W - 1 : x);
	cur_y = y < 0 ? 0 : (y >= VGA_H ? VGA_H - 1 : y);
	vga_update_cursor();
}

void vga_get_cursor(int* x, int* y) {
	*x = cur_x;
	*y = cur_y;
}

void vga_delete_char(void) {
	VGA_MEM[cur_y * VGA_W + cur_x] = (uint16_t)(0x0700 | ' ');
	vga_update_cursor();
}

uint8_t vga_color_attr(const char* name, uint8_t fallback) {
	static const struct {
		const char* n;
		uint8_t c;
	} table[] = {
		{"black", 0},		{"blue", 1},	   {"green", 2},		{"cyan", 3},		   {"red", 4},
		{"magenta", 5},		{"brown", 6},	   {"gray", 7},			{"dark-gray", 8},	   {"dark_gray", 8},
		{"light-blue", 9},	{"light_blue", 9}, {"light-green", 10}, {"light_green", 10},   {"light-cyan", 11},
		{"light_cyan", 11}, {"light-red", 12}, {"light_red", 12},	{"light-magenta", 13}, {"light_magenta", 13},
		{"yellow", 14},		{"white", 15},
	};
	for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++)
		if (strcmp(name, table[i].n) == 0)
			return table[i].c; // foreground on black
	return fallback;
}

// PSP/2 keyboar

static const char kbd_norm[128] = {
	0,	  0,   '1', '2',  '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,	 0,	  'q', 'w', 'e', 'r',
	't',  'y', 'u', 'i',  'o', 'p', '[', ']', 0,   0,	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
	'\'', '`', 0,	'\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,	 '*', 0,   ' ',
};

static const char kbd_shift_table[128] = {
	0,	 0,	  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,	'Q', 'W', 'E', 'R',
	'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,	 0,	  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
	'"', '~', 0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,	 ' ',
};

#define KBD_MAX 255

static volatile int kbd_line_ready;
static int kbd_shift;
static int kbd_ext;
static char kbd_line[KBD_MAX + 1];
static int kbd_len;

void kbd_irq(void) {
	if (kbd_line_ready) {
		(void)inb(0x60); // drop keys while a line waits to be consumed
		return;
	}

	uint8_t sc = inb(0x60);

	if (sc == 0xE0) {
		kbd_ext = 1;
		return;
	}
	if (kbd_ext) { // ignore extended
		kbd_ext = 0;
		return;
	}
	if (sc & 0x80) { // key release
		uint8_t br = sc & 0x7F;
		if (br == 0x2A || br == 0x36)
			kbd_shift = 0;
		return;
	}
	if (sc == 0x2A || sc == 0x36) {
		kbd_shift = 1;
		return;
	}
	if (sc == 0x0E) { // backspace
		if (kbd_len > 0) {
			kbd_len--;
			vga_putc_attr('\b', 0x07);
			vga_delete_char();
		}
		return;
	}
	if (sc == 0x1C) { // enter
		kbd_line[kbd_len] = '\0';
		vga_putc_attr('\n', 0x07);
		kbd_line_ready = 1;
		return;
	}

	char c = kbd_shift ? kbd_shift_table[sc & 0x7F] : kbd_norm[sc & 0x7F];
	if (c && kbd_len < KBD_MAX) {
		kbd_line[kbd_len++] = c;
		vga_putc_attr(c, 0x07);
	}
}

char* kbd_readline(void) {
	while (!kbd_line_ready)
		__asm__ volatile("hlt");
	char* line = strdup(kbd_line);
	kbd_line_ready = 0;
	kbd_len = 0;
	return line;
}

// PIC and IDT

static void pic_init(void) {
	outb(0x20, 0x11);
	outb(0xA0, 0x11); // ICW1: init, cascade, edge
	io_wait();
	outb(0x21, 0x20);
	outb(0xA1, 0x28); // ICW2: irq0->0x20, irq8->0x28
	io_wait();
	outb(0x21, 0x04);
	outb(0xA1, 0x02); // ICW3: slave on irq2
	io_wait();
	outb(0x21, 0x01);
	outb(0xA1, 0x01); // ICW4: 8086 mode
	io_wait();
	outb(0x21, 0xFD); // master: keyboard (irq1) only
	outb(0xA1, 0xFF); // slave: all masked
}

typedef struct __attribute__((packed)) {
	uint16_t base_lo;
	uint16_t sel;
	uint8_t ist;
	uint8_t flags;
	uint16_t base_hi;
} idt_entry_t;

typedef struct __attribute__((packed)) {
	uint16_t limit;
	uint32_t base;
} idt_ptr_t;

extern uint32_t isr_handlers[48];

static idt_entry_t idt[256];
static idt_ptr_t idtr;

void idt_init(void) {
	for (int i = 0; i < 256; i++) {
		uint32_t h = (i < 48) ? isr_handlers[i] : 0;
		idt[i].base_lo = (uint16_t)(h & 0xFFFF);
		idt[i].sel = 0x08; // kernel code segment
		idt[i].ist = 0;
		idt[i].flags = h ? 0x8E : 0x00; // present, ring0 interrupt gate
		idt[i].base_hi = (uint16_t)(h >> 16);
	}
	idtr.limit = sizeof(idt) - 1;
	idtr.base = (uint32_t)idt;
	__asm__ volatile("lidt %0" : : "m"(idtr));
}

static const char* exception_names[32] = {
	"divide error",
	"debug",
	"non-maskable interrupt",
	"breakpoint",
	"overflow",
	"bound range exceeded",
	"invalid opcode",
	"device not available",
	"double fault",
	"coprocessor segment overrun",
	"invalid TSS",
	"segment not present",
	"stack-segment fault",
	"general protection fault",
	"page fault",
	"reserved",
	"x87 FPU error",
	"alignment check",
	"machine check",
	"SIMD exception",
	"virtualization exception",
	"control protection",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
};

void isr_dispatch(struct isr_regs* r) {
	if (r->intno < 32) {
		kprintf("CPU exception %u (%s) err=%u eip=%x cs=%x eflags=%x\n", r->intno, exception_names[r->intno], r->err,
				r->eip, r->cs, r->eflags);
		panic(exception_names[r->intno]);
	}

	// IRQ 32..47
	if (r->intno >= 40)
		outb(0xA0, 0x20); // slave EOI
	outb(0x20, 0x20);	  // master EOI

	if (r->intno == 32 + 1) // IRQ1: keyboard
		kbd_irq();
}

// kernel plumbing

void kprintf(const char* fmt, ...) {
	char buf[512];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	serial_puts(buf);
	vga_write_attr(buf, 0x07);
}

void kdebug(const char* msg) { serial_puts(msg); }

void panic(const char* msg) {
	vga_write_attr("\n!! KERNEL PANIC: ", 0x4F); // fun color
	vga_write_attr(msg, 0x4F);
	vga_write_attr("\n", 0x4F);
	__asm__ volatile("cli");
	for (;;)
		__asm__ volatile("hlt");
}

// libc printf sinks here so quickjs debug output lands on the console
static void putc_sink(const char* s) {
	serial_puts(s);
	vga_write_attr(s, 0x07);
}

void set_output_hook(void (*fn)(const char*));

void kmain(void) {
	serial_init();
	kdebug("typeos: serial up\n");

	vga_clear();
	kdebug("typeos: vga up\n");

	pic_init();
	idt_init();
	kdebug("typeos: interrupts up\n");

	heap_init();
	kdebug("typeos: heap up\n");

	set_output_hook(putc_sink);

	__asm__ volatile("sti");
	kdebug("typeos: boot complete, entering typescript shell\n");

	extern void js_run(void);
	js_run(); // never returns!!

	panic("js_run returned");
}