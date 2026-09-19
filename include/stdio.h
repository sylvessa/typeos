#ifndef TYPEOS_STDIO_H
#define TYPEOS_STDIO_H

#include <stdarg.h>
#include <stddef.h>

typedef void* FILE;
extern FILE* stdout;

int vsnprintf(char* buf, size_t size, const char* fmt, va_list ap);
int snprintf(char* buf, size_t size, const char* fmt, ...);
int sprintf(char* buf, const char* fmt, ...);
int printf(const char* fmt, ...);
int fprintf(FILE* stream, const char* fmt, ...);
int vfprintf(FILE* stream, const char* fmt, va_list ap);
int putchar(int c);
int fputc(int c, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);

#endif