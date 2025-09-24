#include <stdlib.h>

#include <stdio.h>

using namespace _mm_internals;

void *malloc(size_t size) {
	if (size == 0) return NULL;

	printf("malloc'ing memory of size %d\n", size);

	size_t blocks_to_alloc = (size + HEADER_SIZE) / MIN_ALLOC_SIZE;
	if (blocks_to_alloc*MIN_ALLOC_SIZE < size + HEADER_SIZE) {
		++blocks_to_alloc;
	}

	for (size_t i = 0; i < NUM_BLOCKS; ++i) {
		alloc_header_t *header;

		for (size_t j = 0; j < blocks_to_alloc; ++j) {
			if (get_used(i+j)) goto no_alloc;
		}

		for (size_t j = 0; j < blocks_to_alloc; ++j) {
			set_used(i+j);
		}

		header = (alloc_header_t*)get_ptr(i);
		header->s.num_blocks = blocks_to_alloc;

		return header+1;

	no_alloc:;
	}

	// didn't find a large enough contiguous block of free memory
	puts("kernel: panic: couldn't allocate required memory!");
	abort();
}
