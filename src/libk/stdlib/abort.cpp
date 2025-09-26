#include <stdlib.h>

#include <stdio.h>
#include "blit.hpp"

__attribute__((__noreturn__))
void abort(void) {
	BLT_WRITE_STR("kernel: panic: abort()", 22);
	BLT_NEWLINE();
	asm volatile("cli");
	while (1) { asm volatile("hlt"); }
	__builtin_unreachable();
}
