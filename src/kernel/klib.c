#include "klib.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>

// memory

extern unsigned char __kernel_end[];

#ifndef HEAP_END_ADDR
#define HEAP_END_ADDR ((uintptr_t)0x400000UL) // 4 MiB top for the kernel heap
#endif

typedef struct block {
	uint32_t size; // usable payload size in bytes
	uint32_t free;
	struct block* next;
	uint32_t pad;
} block_t;

static block_t* heap_head;

#define HEADER_SIZE ((uintptr_t)sizeof(block_t))
#define ALIGN16(n) (((uintptr_t)(n) + 15) & ~(uintptr_t)15)

void heap_init(void) {
	uintptr_t start = ALIGN16((uintptr_t)__kernel_end);
	uintptr_t end = HEAP_END_ADDR;
	if (end > start + 2 * HEADER_SIZE) {
		heap_head = (block_t*)start;
		heap_head->size = (uint32_t)(end - start - HEADER_SIZE);
		heap_head->free = 1;
		heap_head->next = NULL;
		heap_head->pad = 0;
	}
}

void* malloc(size_t size) {
	if (!heap_head)
		return NULL;
	size_t want = (size_t)ALIGN16(size ? size : 16);

	for (block_t* b = heap_head; b; b = b->next) {
		if (!b->free || b->size < want)
			continue;
		// split if the remainder can hold another block

		if (b->size >= want + 2 * HEADER_SIZE) {
			block_t* nb = (block_t*)((uintptr_t)b + HEADER_SIZE + want);
			nb->size = b->size - want - HEADER_SIZE;
			nb->free = 1;
			nb->next = b->next;
			nb->pad = 0;
			b->next = nb;
		}
		b->size = want;
		b->free = 0;
		return (void*)((uintptr_t)b + HEADER_SIZE);
	}
	return NULL;
}

void free(void* ptr) {
	if (!ptr)
		return;
	block_t* b = (block_t*)((uintptr_t)ptr - HEADER_SIZE);
	b->free = 1;
	// coalesce

	while (b->next && b->next->free) {
		b->size += HEADER_SIZE + b->next->size;
		b->next = b->next->next;
	}
}

void* calloc(size_t n, size_t size) {
	size_t total = n * size;
	void* p = malloc(total);
	if (p)
		memset(p, 0, total);
	return p;
}

void* realloc(void* ptr, size_t size) {
	if (!ptr)
		return malloc(size);
	block_t* b = (block_t*)((uintptr_t)ptr - HEADER_SIZE);
	size_t want = (size_t)ALIGN16(size ? size : 16);

	if (want <= b->size) {
		b->size = want;
		return ptr;
	}

	if (b->next && b->next->free && b->size + HEADER_SIZE + b->next->size >= want) {
		size_t room = b->size + HEADER_SIZE + b->next->size;
		block_t* nn = b->next->next;
		b->size = want;
		if (room > want + 2 * HEADER_SIZE) {
			block_t* nb = (block_t*)((uintptr_t)b + HEADER_SIZE + want);
			nb->size = room - want - HEADER_SIZE;
			nb->free = 1;
			nb->next = nn;
			nb->pad = 0;
			b->next = nb;
		} else {
			b->next = nn;
		}
		return ptr;
	}
	void* np = malloc(want);
	if (!np)
		return NULL;
	memcpy(np, ptr, b->size > want ? want : b->size);
	free(ptr);
	return np;
}

size_t klib_malloc_usable_size(const void* ptr) {
	if (!ptr)
		return 0;
	const block_t* b = (const block_t*)((uintptr_t)ptr - HEADER_SIZE);
	return b->size;
}

// strings

void* memcpy(void* dst, const void* src, size_t n) {
	uint8_t* d = dst;
	const uint8_t* s = src;
	while (n--)
		*d++ = *s++;
	return dst;
}

void* memmove(void* dst, const void* src, size_t n) {
	uint8_t* d = dst;
	const uint8_t* s = src;
	if (d == s || n == 0)
		return dst;
	if (d < s) {
		while (n--)
			*d++ = *s++;
	} else {
		d += n;
		s += n;
		while (n--)
			*--d = *--s;
	}
	return dst;
}

void* memset(void* s, int c, size_t n) {
	uint8_t* p = s;
	while (n--)
		*p++ = (uint8_t)c;
	return s;
}

int memcmp(const void* a, const void* b, size_t n) {
	const uint8_t *p = a, *q = b;
	while (n--) {
		if (*p != *q)
			return *p - *q;
		p++;
		q++;
	}
	return 0;
}

void* memchr(const void* s, int c, size_t n) {
	const uint8_t* p = s;
	uint8_t ch = (uint8_t)c;
	while (n--) {
		if (*p == ch)
			return (void*)p;
		p++;
	}
	return NULL;
}

size_t strlen(const char* s) {
	const char* p = s;
	while (*p)
		p++;
	return (size_t)(p - s);
}

int strcmp(const char* a, const char* b) {
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return (uint8_t)*a - (uint8_t)*b;
}

int strncmp(const char* a, const char* b, size_t n) {
	while (n-- && *a && *a == *b) {
		a++;
		b++;
	}
	return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

char* strchr(const char* s, int c) {
	char ch = (char)c;
	for (;; s++) {
		if (*s == ch)
			return (char*)s;
		if (!*s)
			return NULL;
	}
}

char* strrchr(const char* s, int c) {
	const char* last = NULL;
	char ch = (char)c;
	for (; *s; s++)
		if (*s == ch)
			last = s;
	return (char*)last;
}

char* strpbrk(const char* s, const char* accept) {
	for (; *s; s++)
		if (strchr(accept, *s))
			return (char*)s;
	return NULL;
}

char* strstr(const char* hay, const char* needle) {
	if (!*needle)
		return (char*)hay;
	for (; *hay; hay++) {
		const char *h = hay, *n = needle;
		while (*h && *n && *h == *n) {
			h++;
			n++;
		}
		if (!*n)
			return (char*)hay;
	}
	return NULL;
}

char* strcpy(char* dst, const char* src) {
	char* d = dst;
	while ((*d++ = *src++))
		;
	return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
	size_t i = 0;
	for (; i < n && src[i]; i++)
		dst[i] = src[i];
	for (; i < n; i++)
		dst[i] = '\0';
	return dst;
}

char* strdup(const char* s) {
	size_t n = strlen(s) + 1;
	char* p = malloc(n);
	if (p)
		memcpy(p, s, n);
	return p;
}

char* strndup(const char* s, size_t n) {
	size_t len = 0;
	while (len < n && s[len])
		len++;
	char* p = malloc(len + 1);
	if (!p)
		return NULL;
	memcpy(p, s, len);
	p[len] = '\0';
	return p;
}

size_t strcspn(const char* s, const char* reject) {
	size_t n = 0;
	while (s[n] && !strchr(reject, s[n]))
		n++;
	return n;
}

size_t strspn(const char* s, const char* accept) {
	size_t n = 0;
	while (s[n] && strchr(accept, s[n]))
		n++;
	return n;
}

// ============================ printf ============================

int errno;

static void (*output_hook)(const char* s);

void set_output_hook(void (*fn)(const char* s)) { output_hook = fn; }

static int emit(char** out, size_t* rem, char c) {
	if (*rem > 1) {
		*(*out)++ = c;
		(*rem)--;
	}
	return 1;
}

static int emit_str(char** out, size_t* rem, const char* s, size_t len) {
	size_t i;
	for (i = 0; i < len; i++)
		emit(out, rem, s[i]);
	return (int)len;
}

static char* utoa_buf(unsigned long long v, char* buf, int base, int upper) {
	static const char* digits_l = "0123456789abcdefghijklmnopqrstuvwxyz";
	static const char* digits_u = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const char* digits = upper ? digits_u : digits_l;
	char tmp[40];
	int i = 0;
	if (v == 0)
		tmp[i++] = '0';
	while (v) {
		tmp[i++] = digits[v % (unsigned)base];
		v /= (unsigned)base;
	}
	while (i)
		*buf++ = tmp[--i];
	*buf = '\0';
	return buf;
}

int vsnprintf(char* buf, size_t size, const char* fmt, va_list ap) {
	char* out = buf;
	size_t rem = size;
	int total = 0;

	for (const char* p = fmt; *p; p++) {
		if (*p != '%') {
			emit(&out, &rem, *p);
			total++;
			continue;
		}
		p++;
		if (*p == '%') {
			emit(&out, &rem, '%');
			total++;
			continue;
		}

		// flags

		int left = 0, zero = 0, alt = 0, plus = 0, space = 0;
		for (;; p++) {
			if (*p == '-')
				left = 1;
			else if (*p == '0')
				zero = 1;
			else if (*p == '#')
				alt = 1;
			else if (*p == '+')
				plus = 1;
			else if (*p == ' ')
				space = 1;
			else
				break;
		}

		// width

		int width = 0;
		if (*p == '*') {
			width = va_arg(ap, int);
			if (width < 0) {
				left = 1;
				width = -width;
			}
			p++;
		} else {
			while (*p >= '0' && *p <= '9') {
				width = width * 10 + (*p - '0');
				p++;
			}
		}

		// precision

		int prec = -1;
		if (*p == '.') {
			p++;
			prec = 0;
			if (*p == '*') {
				prec = va_arg(ap, int);
				p++;
			} else {
				while (*p >= '0' && *p <= '9') {
					prec = prec * 10 + (*p - '0');
					p++;
				}
			}
		}

		// length

		int llong = 0, llong1 = 0;
		if (*p == 'l') {
			if (p[1] == 'l') {
				llong = 1;
				p += 2;
			} else {
				llong1 = 1;
				p++;
			}
		} else if (*p == 'h') {
			if (p[1] == 'h')
				p += 2;
			else
				p++;
		} else if (*p == 'z' || *p == 't') {
			p++;
		}

		char conv = *p;
		if (!conv)
			break;

		if (conv == 'c') {
			char c = (char)va_arg(ap, int);
			int n = emit(&out, &rem, c);
			total += n;
			continue;
		}
		if (conv == 's') {
			const char* s = va_arg(ap, const char*);
			if (!s)
				s = "(null)";
			size_t len = strlen(s);
			if (prec >= 0 && (size_t)prec < len)
				len = (size_t)prec;
			int pad = (width > (int)len) ? width - (int)len : 0;
			if (!left)
				while (pad--) {
					emit(&out, &rem, ' ');
					total++;
				}
			total += emit_str(&out, &rem, s, len);
			if (left)
				while (pad-- > 0) {
					emit(&out, &rem, ' ');
					total++;
				}
			continue;
		}
		if (conv == 'p') {
			unsigned long v = (unsigned long)va_arg(ap, void*);
			char tmp[16];
			char* end = utoa_buf(v, tmp, 16, 0);
			int n = (int)(end - tmp) + 2; // 0x prefix
			int pad = (width > n) ? width - n : 0;
			if (!left)
				while (pad--) {
					emit(&out, &rem, ' ');
					total++;
				}
			emit(&out, &rem, '0');
			emit(&out, &rem, 'x');
			total += (int)(end - tmp) + 2;
			emit_str(&out, &rem, tmp, (size_t)(end - tmp));
			if (left)
				while (pad-- > 0) {
					emit(&out, &rem, ' ');
					total++;
				}
			continue;
		}

		int is_float = (conv == 'f' || conv == 'F' || conv == 'e' || conv == 'E' || conv == 'g' || conv == 'G');
		if (is_float) {
			double d = va_arg(ap, double);
			int pdig = (prec >= 0) ? prec : 6;
			// quickjs formats numbers via dtoa lol

			int neg = 0;
			if (d < 0) {
				neg = 1;
				d = -d;
			}
			unsigned long long ip = (unsigned long long)d;
			double frac = d - (double)ip;
			uint32_t fdigits[16];
			int nd = 0;
			for (int i = 0; i < pdig && i < 16; i++) {
				frac *= 10.0;
				uint32_t dig = (uint32_t)frac;
				fdigits[nd++] = dig;
				frac -= (double)dig;
			}
			char body[128];
			char* bp = body;
			if (neg)
				*bp++ = '-';
			char* ipend = utoa_buf(ip, bp, 10, 0);
			bp = ipend;
			*bp++ = '.';
			for (int i = 0; i < nd; i++)
				*bp++ = (char)('0' + fdigits[i]);
			for (int i = nd; i < pdig; i++)
				*bp++ = '0';
			*bp = '\0';
			size_t len = (size_t)(bp - body);
			int pad = (width > (int)len) ? width - (int)len : 0;
			if (!left)
				while (pad--) {
					emit(&out, &rem, ' ');
					total++;
				}
			total += (int)len;
			emit_str(&out, &rem, body, len);
			if (left)
				while (pad-- > 0) {
					emit(&out, &rem, ' ');
					total++;
				}
			continue;
		}

		// integer conversions

		int base = 10;
		int upper = 0;
		if (conv == 'x' || conv == 'X') {
			base = 16;
			upper = (conv == 'X');
		} else if (conv == 'o')
			base = 8;

		unsigned long long v;
		int is_signed = (conv == 'd' || conv == 'i');
		if (llong) {
			v = is_signed ? (unsigned long long)(long long)va_arg(ap, long long)
						  : (unsigned long long)va_arg(ap, unsigned long long);
		} else if (llong1) {
			v = is_signed ? (unsigned long long)(long)va_arg(ap, long) : (unsigned long long)va_arg(ap, unsigned long);
		} else {
			v = is_signed ? (unsigned long long)(int)va_arg(ap, int) : (unsigned long long)va_arg(ap, unsigned int);
		}

		int neg = is_signed && (long long)v < 0;
		if (neg)
			v = (unsigned long long)(-(long long)v);

		char tmp[40];
		char* end = utoa_buf(v, tmp, base, upper);
		size_t dlen = (size_t)(end - tmp);

		// precision zero pads digits

		int zcnt = 0;
		if (prec > (int)dlen)
			zcnt = prec - (int)dlen;
		int prefix = 0;
		if (alt && base == 16 && v != 0)
			prefix = 2; // 0x
		if (alt && base == 8 && v != 0)
			prefix = 1; // 0
		int sign = (neg || plus) ? 1 : (space ? 1 : 0);
		if (neg)
			sign = 1;

		size_t body_len = (size_t)sign + (size_t)prefix + (size_t)zcnt + dlen;
		int pad = (width > (int)body_len) ? width - (int)body_len : 0;

		int use_zero = zero && !left && prec < 0;
		if (!left && !use_zero)
			while (pad--) {
				emit(&out, &rem, ' ');
				total++;
			}
		if (neg) {
			emit(&out, &rem, '-');
			total++;
		} else if (plus) {
			emit(&out, &rem, '+');
			total++;
		} else if (space) {
			emit(&out, &rem, ' ');
			total++;
		}
		if (prefix == 2) {
			emit(&out, &rem, '0');
			emit(&out, &rem, upper ? 'X' : 'x');
			total += 2;
		} else if (prefix == 1) {
			emit(&out, &rem, '0');
			total++;
		}
		if (use_zero)
			while (pad--) {
				emit(&out, &rem, '0');
				total++;
			}
		while (zcnt--) {
			emit(&out, &rem, '0');
			total++;
		}
		total += (int)dlen;
		emit_str(&out, &rem, tmp, dlen);
		if (left)
			while (pad-- > 0) {
				emit(&out, &rem, ' ');
				total++;
			}
	}

	if (size > 0)
		*out = '\0';
	return total;
}

int snprintf(char* buf, size_t size, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(buf, size, fmt, ap);
	va_end(ap);
	return n;
}

int sprintf(char* buf, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(buf, (size_t)-1, fmt, ap);
	va_end(ap);
	return n;
}

int printf(const char* fmt, ...) {
	char buf[512];
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (output_hook)
		output_hook(buf);
	return n;
}

FILE* stdout = NULL;

int vfprintf(FILE* stream, const char* fmt, va_list ap) {
	char buf[512];
	int n = vsnprintf(buf, sizeof(buf), fmt, ap);
	if (output_hook && stream == stdout)
		output_hook(buf);
	return n;
}

int fprintf(FILE* stream, const char* fmt, ...) {
	char buf[512];
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (output_hook && stream == stdout)
		output_hook(buf);
	return n;
}

int putchar(int c) {
	char ch = (char)c;
	if (output_hook)
		output_hook(&ch);
	return c;
}

int fputc(int c, FILE* stream) {
	if (stream == stdout)
		return putchar(c);
	(void)stream;
	return c;
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
	(void)stream; // debug dumps only; report success without writing
	return size * nmemb;
}

// misc libc

int gettimeofday(struct timeval* tv, void* tz) {
	(void)tz;
	if (tv) {
		tv->tv_sec = 0; // no RTC support (yet ?)
		tv->tv_usec = 0;
	}
	return 0;
}

time_t time(time_t* t) {
	if (t)
		*t = 0;
	return 0;
}

struct tm* localtime_r(const time_t* t, struct tm* tm) {
	(void)t;

	tm->tm_sec = tm->tm_min = tm->tm_hour = tm->tm_mday = 0;
	tm->tm_mon = tm->tm_year = 0;
	tm->tm_wday = tm->tm_yday = 0;
	tm->tm_isdst = 0;
	tm->tm_gmtoff = 0;
	return tm;
}

int abs(int x) { return x < 0 ? -x : x; }

void abort(void) {
	if (output_hook)
		output_hook("abort() called\n");
	for (;;)
		;
}

void exit(int code) {
	(void)code;
	if (output_hook)
		output_hook("exit() called\n");
	for (;;)
		;
}

void __assert_fail(const char* expr, const char* file, int line) {
	char buf[256];
	snprintf(buf, sizeof(buf), "assertion failed: %s (%s:%d)\n", expr, file, line);
	if (output_hook)
		output_hook(buf);
	for (;;)
		;
}

void qsort(void* base, size_t nmemb, size_t size, int (*cmp)(const void*, const void*)) {
	if (nmemb < 2)
		return;

	char* b = base;

	for (size_t i = 1; i < nmemb; i++) {
		for (size_t j = i; j > 0; j--) {
			char* a = b + (j - 1) * size;
			char* c = b + j * size;
			if (cmp(a, c) <= 0)
				break;
			for (size_t k = 0; k < size; k++) {
				char t = a[k];
				a[k] = c[k];
				c[k] = t;
			}
		}
	}
}

uint64_t __udivmoddi4(uint64_t u, uint64_t v, uint64_t* rp) {
	uint64_t q = 0, r = 0;
	int i;

	if (v == 0)
		return 0;
	for (i = 63; i >= 0; i--) {
		uint64_t carry = r >> 63;
		r = (r << 1) | ((u >> i) & 1);
		if (carry || r >= v) {
			r -= v;
			q |= (uint64_t)1 << i;
		}
	}
	if (rp)
		*rp = r;
	return q;
}

uint64_t __udivdi3(uint64_t a, uint64_t b) { return __udivmoddi4(a, b, 0); }

uint64_t __umoddi3(uint64_t a, uint64_t b) {
	uint64_t r;
	__udivmoddi4(a, b, &r);
	return r;
}

int64_t __divdi3(int64_t a, int64_t b) {
	int neg = (a < 0) != (b < 0);
	uint64_t ua = a < 0 ? (uint64_t)0 - (uint64_t)a : (uint64_t)a;
	uint64_t ub = b < 0 ? (uint64_t)0 - (uint64_t)b : (uint64_t)b;
	uint64_t q = __udivmoddi4(ua, ub, 0);
	int64_t sq = (int64_t)q;
	return neg ? ~(sq - 1) : sq;
}

int64_t __moddi3(int64_t a, int64_t b) {
	uint64_t ua = a < 0 ? (uint64_t)0 - (uint64_t)a : (uint64_t)a;
	uint64_t ub = b < 0 ? (uint64_t)0 - (uint64_t)b : (uint64_t)b;
	uint64_t r;
	int64_t sr;
	__udivmoddi4(ua, ub, &r);
	sr = (int64_t)r;
	return a < 0 ? -sr : sr;
}