#include <stdio.h>

#include "vga.hpp"

int puts(const char *s) {
	term_writestring(s);
	term_putchar('\n');

	return 0;
}
