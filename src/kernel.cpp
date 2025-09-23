/* from https://wiki.osdev.org/Bare_Bones */

#include <stddef.h>
#include <stdint.h>

#include "vga.hpp"

/* Check if the compiler thinks you are targeting the wrong operating system */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with an ix86-elf compiler"
#endif

extern "C" {
/*
 * make the kernel_main function extern "C"
 * bc idk what funky stuff C++ does with calling conventions,
 * this way I know how to call it from assembly
 */
/* also, to avoid name mangling */

void kernel_main(void) {
	/* Initialize terminal interface */
	term_init();

	term_writestring("This first line will be scrolled offscreen\n");
	term_putchar('\n');
	term_putchar('\n');

	/* Newline support is left as an exercise. */
	term_writestring("Hello, kernel world!\n");
	term_writestring("Kyk, ek kan selfs Afrikaans hier skryf! D" "\xA1" "e projek is ");
	term_setcolor(vga_entry_color(
		vga_color(uint8_t(vga_fgcolor(term_getcolor()))^8),
		vga_bgcolor(term_getcolor())
	));
	term_writestring("cool");
	term_resetcolor();
	term_writestring(" omdat:\n");
	term_writestring(" - foo\n");
	term_writestring(" - bar\n");
	term_writestring(" - baz\n");
	term_writestring("Lorum ipsum dolor set...\n");

	term_writestring("\nCharacter set:\n");
	for (uint8_t high_half = 0; high_half < 0xF; ++high_half) {
		constexpr uint16_t indent = 0x2020;
		term_write_raw((uint8_t*)&indent, 2);
		for (uint8_t low_half = 0; low_half < 0xF; ++low_half) {
			term_putbyte(low_half + (high_half << 4));
		}
		term_putchar('\n');
	}

	term_putchar('\n');
}

}
