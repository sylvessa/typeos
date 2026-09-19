#pragma once

#include <stddef.h>

void heap_init(void);
size_t klib_malloc_usable_size(const void* ptr);
void set_output_hook(void (*fn)(const char* s));