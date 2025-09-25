#include <cppsupport.hpp>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

namespace __cxxabiv1 {
#define MAX_DESTRUCTORS 256

	struct destructor_t {
		void (*fn)(void *);
		void *arg;
		void *handle;

		destructor_t(void (*fn)(void *), void *arg, void *handle) : fn(fn), arg(arg), handle(handle) {}
		destructor_t() {}
	};

	static destructor_t destructors[MAX_DESTRUCTORS];
	static size_t num_destructors = 0;

	int __cxa_atexit(void (*destructor)(void *), void *arg, void *__dso_handle) {
		if (num_destructors == MAX_DESTRUCTORS) {
			printf("Too many instructors registered! Maximum is %d\n", MAX_DESTRUCTORS);
			return 1;
		}
		destructors[num_destructors++] = destructor_t(destructor, arg, __dso_handle);
		return 0;
	}
	void __cxa_finalize(void *f) {
		if (f == NULL) {
			for (size_t i = 0; i < num_destructors; ++i) {
				destructors[i].fn(destructors[i].arg);
			}

			num_destructors = 0;

			return;
		}

		for (size_t i = 0; i < num_destructors; ++i) {
			if (destructors[i].fn == (void(*)(void*))f) {
				destructors[i].fn(destructors[i].arg);

				for (size_t j = i+1; j < num_destructors; ++j) {
					destructors[j-1] = destructors[j];
				}

				--num_destructors;

				return;
			}
		}

		printf("Tried calling non-registered destructor!\n");
	}



	// unconditionally acquire the guard, do nothing to release, do nothing
	// to abort. Since I don't plan on adding multithreading, there is no
	// need to implement this properly
	extern "C" int __cxa_guard_acquire(__guard *) {
		return true;
	}
	extern "C" void __cxa_guard_release(__guard *) { }
	extern "C" void __cxa_guard_abort(__guard *) { }
}

void *operator new(size_t size) {
	//printf("new(%u)\n", size);
	return calloc(size, 1);
}
void *operator new[](size_t size) {
	//printf("new[](%u)\n", size);
	return calloc(size, 1);
}
void operator delete(void *p) {
	//printf("delete %p\n", p);
	free(p);
}
void operator delete[](void *p) {
	//printf("delete[] %p\n", p);
	free(p);
}
// no special support for sized deletes (idk why you'd have them honestly??)
void operator delete(void *p, size_t size) {
	(void)size;
	//printf("delete(%u) %p\n", size, p);
	free(p);
}
void operator delete[](void *p, size_t size) {
	(void)size;
	//printf("delete[](%u) %p\n", size, p);
	free(p);
}
