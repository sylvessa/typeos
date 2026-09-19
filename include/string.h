#ifndef TYPEOS_STRING_H
#define TYPEOS_STRING_H

#include <stddef.h>

void* memcpy(void* dst, const void* src, size_t n);
void* memmove(void* dst, const void* src, size_t n);
void* memset(void* s, int c, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);
void* memchr(const void* s, int c, size_t n);

size_t strlen(const char* s);
int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, size_t n);
char* strchr(const char* s, int c);
char* strrchr(const char* s, int c);
char* strpbrk(const char* s, const char* accept);
char* strstr(const char* hay, const char* needle);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, size_t n);
char* strdup(const char* s);
char* strndup(const char* s, size_t n);
size_t strcspn(const char* s, const char* reject);
size_t strspn(const char* s, const char* accept);

#endif