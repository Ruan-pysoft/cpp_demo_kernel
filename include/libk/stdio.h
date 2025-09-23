#pragma once

#include <sys/cdefs.h>

#define EOF (-1)

extern "C" {

int printf(const char *__restrict, ...);
int putchar(int);
int puts(const char *);

}
