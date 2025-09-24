#include <stdlib.h>

#include <stdio.h>
#include <string.h>

using namespace _mm_internals;

void *realloc(void *p, size_t size) {
	if (size == 0) {
		free(p);
		return NULL;
	}
	if (p == NULL) {
		return malloc(size);
	}

	printf("realloc'ing memory at %p to size %d\n", p, size);

	// I could take the easy way out here of malloc, memcpy, free,
	// but honestly I've got such a good (and crazy) memory model going
	// here that that would feel like a waste, so I'm going to actually
	// implement a proper realloc function

	// pointer should point to right after the header structure
	alloc_header_t *header = (alloc_header_t*)p;
	--header;

	// verify the header has 1KiB alignment
	if (size_t(header) & 1023) {
		puts("kernel: panic: passed a pointer to free not from malloc");
		abort();
	}

	const size_t begin = get_idx(header);
	const size_t curr_len = header->s.num_blocks;
	size_t req_len = (size + HEADER_SIZE) / MIN_ALLOC_SIZE;
	if (req_len*MIN_ALLOC_SIZE < size + HEADER_SIZE) ++req_len;

	if (req_len == curr_len) {
		// nothing to do
		return p;
	} else if (req_len < curr_len) {
		// shrink memory

		// mark the memory as free
		for (size_t i = req_len; i < curr_len; ++i) {
			// verify that the memory hasn't already been freed
			if (!get_used(begin+i)) {
				puts("kernel: panic: double free detected!");
				abort();
			}

			set_free(begin+i);
		}

		// Very important! Update the no of allocated chunks in the
		// header
		header->s.num_blocks = req_len;

		return p;
	} else {
		// grow memory
		// this can actually get complicated:
		// if there's enough memory available directly afterwards, we
		// should just grow forwards.
		// otherwise we should malloc + memcpy + free.
		// in theory, we could also try expanding backwards *and*
		// forwards at the same time and using a memmove and our own
		// custom funky stuff with the headers,
		// which should prevent a few cases where malloc wouldn't
		// be able to find enough memory for the realloc but enough
		// was available if you realloc *over* the old memory, but
		// that's *way* too much effort rn

		for (size_t i = curr_len; i < req_len; ++i) {
			if (get_used(begin+i)) {
				// chunk is used, not enough memory directly
				// after, need to use the normal lame way
				goto malloc_memcpy_free;
			}

			set_used(begin+i);
			// update the no. of chunks allocated according to the
			// header already in the loop rather than all at the
			// end, so that if we only get half way through and
			// need to do malloc/memcpy/free, all the chunks
			// now marked as used will be unmarked again by free
			++header->s.num_blocks;
		}

		return p;

	malloc_memcpy_free: // our cool method didn't work :( back to the lame way
		void *new_p = malloc(size);

		if (new_p == NULL) {
			puts("kernel: panic: failed reallocing array");
			abort();
		}

		// note that we don't know the exact size of the allocated
		// data, only the number of 1KiB chunks it has allocated to it;
		// this means that we have to just memcpy over *all* the
		// chunks, which is a bit inefficient. But we've got cpu
		// cycles to spare, and hopefully the previous code block
		// should work.
		//
		// Come to think of it, I should adjust my malloc function
		// to prefer assigning memory chunks far apart rather than
		// adjacent, so that the cool method of reallocing has a
		// better chance of working...
		memcpy(new_p, p, curr_len * MIN_ALLOC_SIZE);

		free(p);

		return new_p;
	}
}
