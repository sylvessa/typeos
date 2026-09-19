#ifndef TYPEOS_STDLIB_H
#define TYPEOS_STDLIB_H

#include <stddef.h>

#define alloca(n) __builtin_alloca(n)

void* malloc(size_t size);
void* calloc(size_t n, size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);
void qsort(void* base, size_t nmemb, size_t size, int (*cmp)(const void*, const void*));
void abort(void) __attribute__((noreturn));
void exit(int code) __attribute__((noreturn));
int abs(int x);

#endif