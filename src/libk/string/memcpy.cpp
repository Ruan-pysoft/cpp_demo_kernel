#include <string.h>

#include <stdint.h>

void *memcpy(void *__restrict dest, const void *__restrict src, size_t n) {
	// TODO: perhaps try optimising by first looping from i = 0 to n/4
	// and copying 32-bit sections over at a time (using uint32_t),
	// then doing the last 0-3 one-byte runs at the end with a case?
	// Also, could 64-bit copying be faster,
	// even though the cpu is in 32-bit mode?
	// Also also, how would alignment screw with stuff?

	if (dest == src) return dest;

	for (size_t i = 0; i < n; ++i) {
		((uint8_t*)dest)[i] = ((const uint8_t*)src)[i];
	}

	return dest;
}
