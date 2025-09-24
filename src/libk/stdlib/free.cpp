#include <stdlib.h>

#include <stdio.h>

void free(void *p) {
	printf("freeing memory at %p\n", p);
	if (!p) return;
	puts("kernel: panic: free not implemented yet!");
	abort();
}
