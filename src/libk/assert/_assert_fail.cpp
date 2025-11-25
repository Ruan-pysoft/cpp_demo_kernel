#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include "blit.hpp"

void _assert_fail(const char *assertion, const char *file, int line, const char *func) {
	BLT_WRITE_STR(file, strlen(file));
	BLT_WRITE_CHR(':');
	(void) line; // TODO: write line nr.
	BLT_WRITE_STR(": ", 2);
	BLT_WRITE_STR(func, strlen(func));
	BLT_WRITE_STR(": Assertion `", 13);
	BLT_WRITE_STR(assertion, strlen(assertion));
	BLT_WRITE_STR("` failed.", 11);
	abort();
}
