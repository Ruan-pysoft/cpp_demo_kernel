#include <stdio.h>

#include "vga.hpp"

int putchar(int c) {
	term_putchar(c);

	return c;
}
