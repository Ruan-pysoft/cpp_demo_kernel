#include <stdlib.h>

#include <stdio.h>

void *malloc(size_t size) {
	if (size == 0) return NULL;

	printf("malloc'ing memory of size %d\n", size);
	puts("kernel: panic: malloc not implemented yet!");
	abort();
}
