#include <stdlib.h>

#include <stdio.h>

using namespace _mm_internals;

void free(void *p) {
	//printf("freeing memory at %p\n", p);
	if (!p) return;

	// pointer should point to right after the header structure
	alloc_header_t *header = (alloc_header_t*)p;
	--header;

	// verify the header has 1KiB alignment
	if (size_t(header) & 1023) {
		puts("kernel: panic: passed a pointer to free not from malloc");
		abort();
	}

	const size_t begin = get_idx(header);
	const size_t len = header->s.num_blocks;

	// mark the memory as free
	for (size_t i = 0; i < len; ++i) {
		// verify that the memory hasn't already been freed
		if (!get_used(begin+i)) {
			puts("kernel: panic: double free detected!");
			abort();
		}

		set_free(begin+i);
	}
}
