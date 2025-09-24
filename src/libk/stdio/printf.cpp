#include <stdio.h>

#include <limits.h>
#include <stdarg.h>

#include <string.h>

#include "vga.hpp"

static int print_uint32(uint32_t u) {
	// stolen from https://git.sr.ht/~ruan_p/ministdlib/tree/master/item/src/ministd_fmt.c#L82
	// (code written by me for my custom c runtime)

	/* max int is 2^32-1 ~= 10^9 * 4, so no more than 10 digits */
	char buf[10];
	size_t len = 0;

	if (u == 0) {
		putchar('0');
		return 1;
	}

	// construct the number backwards, starting with the least significant
	// digit, because it's just easier that way
	len = 0;
	while (len <= 10 && u > 0) {
		buf[10-len] = '0' + u%10;
		u /= 10;
		++len;
	}

	term_write(&buf[10-len + 1], len);

	return len;
}
static int print_int32(int32_t d) {
	// stolen from https://git.sr.ht/~ruan_p/ministdlib/tree/master/item/src/ministd_fmt.c#L50
	// (code written by me for my custom c runtime)

	/* max int is 2^31-1 ~= 10^9 * 2, so no more than 10 digits */
	char buf[10];
	size_t len = 0;

	if (d == 0) {
		putchar('0');
		return 1;
	}

	if (uint32_t(d) == (~uint32_t(d))+1) {
		/* d == -2^31 */
		putchar('-');
		return 1 + print_uint32(uint32_t(d));;
	}

	if (d < 0) {
		putchar('-');;
		return 1 + print_int32(d);
	}

	// construct the number backwards, starting with the least significant
	// digit, because it's just easier that way
	len = 0;
	while (len <= 10 && d > 0) {
		buf[10-len] = '0' + d%10;
		d /= 10;
		++len;
	}

	term_write(&buf[10-len + 1], len);

	return len;
}
static int print_hex32(int32_t x) {
	// stolen from https://git.sr.ht/~ruan_p/ministdlib/tree/master/item/src/ministd_fmt.c#L50
	// (code written by me for my custom c runtime)

	/* max int is 2^32-1 = 16^8-1, so no more than 8 digits */
	char buf[8];
	size_t len = 0;

	if (x == 0) {
		putchar('0');
		return 1;
	}

	len = 0;
	while (len <= 10 && x > 0) {
		const uint8_t digit = x&0xF;

		if (digit < 10) buf[8-len] = '0' + digit;
		else buf[8-len] = 'a' - 10 + digit;

		x >>= 4;
		++len;
	}

	term_write(&buf[8-len + 1], len);

	return len;
}
static int print_pointer(void *p) {
	// stolen from https://git.sr.ht/~ruan_p/ministdlib/tree/master/item/src/ministd_fmt.c#L50
	// (code written by me for my custom c runtime)

	size_t x = (size_t)p;

	/* max int is 2^32-1 = 16^8-1, so 8 digits */
	char buf[8];

	for (size_t len = 1; len <= 8; ++len) {
		const uint8_t digit = x&0xF;

		if (digit < 10) buf[8-len] = '0' + digit;
		else buf[8-len] = 'a' - 10 + digit;

		x >>= 4;
	}

	term_writestring("0x");
	term_write(buf, 8);

	return 10;
}

int printf(const char *__restrict format, ...) {
	/*
	 * idk how varargs work and can't be arsed to work out how to parse
	 * printf format specifiers, so all of this code comes from
	 * https://wiki.osdev.org/Meaty_Skeleton
	 */
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0') {
		const size_t maxrem = INT_MAX - written;

		// no format specifier, incl escaped '%'
		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%') ++format;

			size_t amount = 1;
			while (format[amount] && format[amount] != '%') ++amount;
			if (maxrem < amount) {
				// TODO: kernel log here?
				return -1; // overflow
			}

			term_write(format, amount);

			format += amount;
			written += amount;
			continue;
		}

		const char *const format_begun_at = format++;

		if (*format == 'c') {
			++format;

			const char c = (char)va_arg(parameters, int /* char promotes to an int, for some reason?? */);
			if (!maxrem) return -1; // overflow

			term_putchar(c);

			++written;
		} else if (*format == 's') {
			++format;

			const char *const str = va_arg(parameters, const char *);
			const size_t len = strlen(str);
			if (maxrem < len) return -1; // overflow

			term_write(str, len);

			++written;
		} else if (*format == 'u') {
			++format;

			const uint32_t u = (uint32_t)va_arg(parameters, uint32_t);
			/* max int is 2^31-1 ~= 10^9 * 2, so no more than 10 digits */
			if (maxrem < 10) return -1; // overflow

			written += print_uint32(u);
		} else if (*format == 'd') {
			++format;

			const int32_t d = (int32_t)va_arg(parameters, int32_t);
			/* max int is 2^31-1 ~= 10^9 * 2, so no more than 10 digits */
			/* can also be negative, so add an extra character for the sign */
			if (maxrem < 11) return -1; // overflow

			written += print_int32(d);
		} else if (*format == 'x') {
			++format;

			const uint32_t x = (uint32_t)va_arg(parameters, uint32_t);
			/* max int is 2^31-1 ~= 10^9 * 2, so no more than 10 digits */
			/* can also be negative, so add an extra character for the sign */
			if (maxrem < 8) return -1; // overflow

			written += print_hex32(x);
		} else if (*format == 'p') {
			++format;

			void *const p = (void*)va_arg(parameters, void*);
			/* max int is 2^31-1 ~= 10^9 * 2, so no more than 10 digits */
			/* can also be negative, so add an extra character for the sign */
			if (maxrem < 10) return -1; // overflow

			written += print_pointer(p);
		} else {
			// if unknown format specifier, seems we just give up?

			format = format_begun_at;

			size_t len = strlen(format);
			if (maxrem < len) return -1;

			term_write(format, len);

			written += len;
			format += len;
		}
	}

	va_end(parameters);
	return written;
}
