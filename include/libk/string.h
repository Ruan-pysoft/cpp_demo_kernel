#pragma once

#include <sys/cdefs.h>

#include <stddef.h>

extern "C" {

int memcmp(const void *, const void *, size_t);
void *memcpy(void *__restrict, const void *__restrict, size_t);
void *memmove(void *, const void *, size_t n);
void *memset(void *, int, size_t);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *);

}
