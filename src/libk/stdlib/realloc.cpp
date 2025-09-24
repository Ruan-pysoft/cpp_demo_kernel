#include <stdlib.h>

#include <stdio.h>

void *realloc(void *p, size_t size) {
	if (size == 0) {
		free(p);
		return NULL;
	}
	if (p == NULL) {
		return malloc(size);
	}

	printf("realloc'ing memory at %p to size %d\n", p, size);
	puts("kernel: panic: realloc not implemented yet!");
	abort();
}
