#include <stdlib.h>

#include <assert.h>
#include <stdio.h>

using namespace _mm_internals;

void free(void *p) {
	//printf("freeing memory at %p\n", p);
	if (!p) return;

	// pointer should point to right after the header structure
	alloc_header_t *header = (alloc_header_t*)p;
	--header;

	// verify the header has 1KiB alignment
	assert((size_t(header)&1023) == 0 && "passed a pointer to free that isn't from malloc");

	const size_t begin = get_idx(header);
	const size_t len = header->s.num_blocks;

	// mark the memory as free
	for (size_t i = 0; i < len; ++i) {
		// verify that the memory hasn't already been freed
		assert(get_used(begin+i) && "double free detected!");

		set_free(begin+i);
	}
}
