#include <stdio.h>

#include "vga.hpp"

int puts(const char *s) {
	term::writestring(s);
	term::putchar('\n');

	return 0;
}
