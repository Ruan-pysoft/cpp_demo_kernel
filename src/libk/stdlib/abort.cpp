#include <stdlib.h>

#include <stdio.h>

__attribute__((__noreturn__))
void abort(void) {
	puts("kernel: panic: abort()");
	while (1) { asm volatile("hlt"); }
	__builtin_unreachable();
}
