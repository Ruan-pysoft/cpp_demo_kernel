#include <string.h>

int strcmp(const char *s1, const char *s2) {
	size_t i;
	for (i = 0; s1[i] != 0 && s2[i] != 0; ++i) {
		if (s1[i] != s2[i]) return (int)s1[i] - s2[i];
	}
	if (s1[i] == 0) return -(int)s2[i];
	else if (s2[i] == 0) return s1[i];
	else return 0;
}
