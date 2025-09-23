#include <stdlib.h>

void abort(void) {
	asm volatile(
		"cli\n"
		".stdlib_abort_hltloop: hlt\n"
		"jmp .stdlib_abort_hltloop\n"
		:::
	);

	__builtin_unreachable();
}
