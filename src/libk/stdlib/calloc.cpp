#include <stdlib.h>

#include <stdio.h>
#include <string.h>

void *calloc(size_t n, size_t size) {
	void *res = malloc(n*size); // should technically check for overflow...

	memset(res, 0, n*size);

	return res;
}
