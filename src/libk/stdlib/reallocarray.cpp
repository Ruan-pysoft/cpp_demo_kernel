#include <stdlib.h>

#include <stdio.h>

void *reallocarray(void *p, size_t n, size_t size) {
	return realloc(p, n*size); // should technically check for overflow...
}
