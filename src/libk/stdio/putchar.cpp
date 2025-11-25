#include <stdio.h>

#include "vga.hpp"

int putchar(int c) {
	term::putchar(c);

	return c;
}
