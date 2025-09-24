#pragma once

#include <stddef.h>
#include <sys/cdefs.h>

extern "C" {

__attribute__((__noreturn__))
void abort(void);

void *malloc(size_t);
void free(void *);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void *reallocarray(void *, size_t, size_t);

}
