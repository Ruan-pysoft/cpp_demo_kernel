#pragma once

// https://wiki.osdev.org/C%2B%2B

#include <stddef.h>

namespace __cxxabiv1 {
	// for registering global destructors
	extern void *__dso_handle;
	extern "C" int __cxa_atexit(void (*destructor)(void *), void *arg, void *__dso_handle);
	extern "C" void __cxa_finalize(void *f);

	// for local static variable support

	__extension__ typedef int __guard __attribute__((mode(__DI__)));

	extern "C" int __cxa_guard_acquire(__guard *);
	extern "C" void __cxa_guard_release(__guard *);
	extern "C" void __cxa_guard_abort(__guard *);
}

void *operator new(size_t);
void *operator new[](size_t);
void operator delete(void *);
void operator delete[](void *);
void operator delete(void *, size_t);
void operator delete[](void *, size_t);
