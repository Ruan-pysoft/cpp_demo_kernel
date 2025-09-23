#include <string.h>

int memcmp(const void *s1, const void *s2, size_t n) {
	const unsigned char *const b1 = (const unsigned char *)s1;
	const unsigned char *const b2 = (const unsigned char *)s2;
	for (size_t i = 0; i < n; ++i) {
		if (b1[i]-b2[i]) return b1[i]-b2[i];
	}

	return 0;
}
