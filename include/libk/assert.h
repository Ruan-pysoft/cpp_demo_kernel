#pragma once

extern "C" {

void _assert_fail(const char *assertion, const char *file, int line, const char *func);
#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr) ((expr) ? (void)0 : _assert_fail(#expr, __FILE__, __LINE__, __func__))
#endif

}
