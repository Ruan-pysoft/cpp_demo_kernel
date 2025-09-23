#include <string.h>

#include <stdint.h>

void *memmove(void *dest, const void *src, size_t n) {
	// TODO: perhaps try optimising by first looping from i = 0 to n/4
	// and copying 32-bit sections over at a time (using uint32_t),
	// then doing the last 0-3 one-byte runs at the end with a case?
	// Also, could 64-bit copying be faster,
	// even though the cpu is in 32-bit mode?
	// Also also, how would alignment screw with stuff?

	if (dest == src) return dest;

	if (dest < src) {
		// copy low bytes first
		for (size_t i = 0; i < n; ++i) {
			((uint8_t*)dest)[i] = ((const uint8_t*)src)[i];
		}
	} else {
		// copy high bytes first
		size_t i = n;
		while (i --> 0) { // safe down-iter to 0; might be unneeded (is size_t signed?)
			((uint8_t*)dest)[i] = ((const uint8_t*)src)[i];
		}
	}

	return dest;
}
