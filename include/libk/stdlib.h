#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

namespace _mm_internals {
	// free memory start at 16MiB
	static constexpr size_t FREE_MEM_LOW_ADDR = 0x01'00'00'00;
	// assume there is at least 256MiB of free memory
	// any more will not be used,
	// any less will likely cause a crash or something
	static constexpr size_t FREE_MEM_CAP = 0x10'00'00'00;

	// stolen from https://git.sr.ht/~ruan_p/ministdlib/tree/master/item/src/ministd_memory.c#L42
	using ALIGN = uint8_t[8];
	union alloc_header_t {
		struct {
			size_t num_blocks;
		} s;
		ALIGN stub;
	};
	static constexpr size_t HEADER_SIZE = sizeof(alloc_header_t);

	// only allocate 1KiB+ of memory
	static constexpr size_t MIN_ALLOC_SIZE = 1024;
	static constexpr size_t NUM_BLOCKS = FREE_MEM_CAP / MIN_ALLOC_SIZE;
	// bitset to keep track of free blocks of memory
	extern uint8_t is_used[NUM_BLOCKS / 8];

	static inline bool get_used(size_t block_idx) {
		// test the (block_idx % 8)'th bit of the (block_dix / 8)'th byte
		return (is_used[block_idx/8] >> (block_idx&7)) & 1;
	}
	static inline void set_free(size_t block_idx) {
		// reset the (block_idx % 8)'th bit of the (block_dix / 8)'th byte
		is_used[block_idx/8] &= ~uint8_t(1 << (block_idx&7));
	}
	static inline void set_used(size_t block_idx) {
		// set the (block_idx % 8)'th bit of the (block_dix / 8)'th byte
		is_used[block_idx/8] |= 1 << (block_idx&7);
	}
	static inline void *get_ptr(size_t block_idx) {
		return (void*)(FREE_MEM_LOW_ADDR + MIN_ALLOC_SIZE*block_idx);
	}
	static inline size_t get_idx(void *ptr) {
		return (size_t(ptr) - FREE_MEM_LOW_ADDR) / MIN_ALLOC_SIZE;
	}
}

extern "C" {

__attribute__((__noreturn__))
void abort(void);

void *malloc(size_t);
void free(void *);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void *reallocarray(void *, size_t, size_t);

}
