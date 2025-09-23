/* from https://wiki.osdev.org/Bare_Bones */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Check if the compiler thinks you are targeting the wrong operating system */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with an ix86-elf compiler"
#endif

/* Hardware text mode color constants. */
enum class vga_color : uint8_t {
	black = 0,
	blue = 1,
	green = 2,
	cyan = 3,
	red = 4,
	magenta = 5,
	brown = 6,
	light_grey = 7,
	dark_grey = 8,
	light_blue = 9,
	light_green = 10,
	light_cyan = 11,
	light_red = 12,
	light_magenta = 13,
	light_brown = 14,
	white = 15,
};

static inline uint8_t vga_entry_color(vga_color fg, vga_color bg) {
	return uint8_t(fg) | (uint8_t(bg) << 4);
}

static inline uint16_t vga_entry(uint8_t b, uint8_t color) {
	return uint16_t(b) | (uint16_t(color) << 8);
}

size_t strlen(const char *str) {
	size_t len = 0;
	while (str[len]) ++len;
	return len;
}

constexpr size_t VGA_WIDTH  = 80;
constexpr size_t VGA_HEIGHT = 25;
constexpr size_t VGA_ADDR   = 0xB8000;

size_t terminal_row;
size_t terminal_col;
uint8_t terminal_color;
uint16_t *const terminal_buffer = (uint16_t*)VGA_ADDR;

void terminal_initialize() {
	terminal_row = 0;
	terminal_col = 0;
	terminal_color = vga_entry_color(vga_color::light_grey, vga_color::black);

	for (size_t y = 0; y < VGA_HEIGHT; ++y) {
		for (size_t x = 0; x < VGA_WIDTH; ++x) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolour(uint8_t color) {
	terminal_color = color;
}

void terminal_putentryat(uint8_t b, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(b, color);
}

void terminal_putbyte(uint8_t b) {
	terminal_putentryat(b, terminal_color, terminal_col, terminal_row);
	if (++terminal_col == VGA_WIDTH) {
		terminal_col = 0;
		if (++terminal_row == VGA_HEIGHT) terminal_row = 0;
	}
}
void terminal_putchar(char c) {
	if (c == '\n') {
		terminal_col = 0;
		if (++terminal_row == VGA_HEIGHT) terminal_row = 0;
	} else {
		terminal_putbyte(uint8_t(c));
	}
}

void terminal_write(const char *data, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		terminal_putchar(data[i]);
	}
}

void terminal_writestring(const char *data) {
	terminal_write(data, strlen(data));
}

extern "C" {
/*
 * make the kernel_main function extern "C"
 * bc idk what funky stuff C++ does with calling conventions,
 * this way I know how to call it from assembly
 */
/* also, to avoid name mangling */

void kernel_main(void) {
	/* Initialize terminal interface */
	terminal_initialize();

	/* Newline support is left as an exercise. */
	terminal_writestring("Hello, kernel world!\n");
	terminal_writestring("Kyk, ek kan selfs Afrikaans hier skryf! D" "\xA1" "e projek is \"cool\" omdat:\n");
	terminal_writestring(" - foo\n");
	terminal_writestring(" - bar\n");
	terminal_writestring(" - baz\n");
	terminal_writestring("Lorum ipsum dolor set...\n");

	terminal_writestring("\nCharacter set:\n");
	for (uint8_t high_half = 0; high_half < 0xF; ++high_half) {
		terminal_putchar(' ');
		terminal_putchar(' ');
		for (uint8_t low_half = 0; low_half < 0xF; ++low_half) {
			terminal_putbyte(low_half + (high_half << 4));
		}
		terminal_putchar('\n');
	}
}

}
