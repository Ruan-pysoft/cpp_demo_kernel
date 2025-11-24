/* from https://wiki.osdev.org/Bare_Bones */

#include <stddef.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "idt.hpp"
#include "vga.hpp"
#include "pic.hpp"
#include "pit.hpp"
#include "ps2.hpp"
#include "blit.hpp"
#include "ioport.hpp"
#include "gdt.hpp"

/* Check if the compiler thinks you are targeting the wrong operating system */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with an ix86-elf compiler"
#endif

/*
 * make the kernel_main function extern "C"
 * bc idk what funky stuff C++ does with calling conventions,
 * this way I know how to call it from assembly
 */
/* also, to avoid name mangling */
extern "C" void kernel_early_main(void);
extern "C" void kernel_main(void);

void kernel_early_main() {
	__asm__ volatile("cli" ::: "memory");

	/* Initialize terminal interface */
	term_init();

	gdt::init();
	gdt::load();

	idt::init();
	idt::load();

	/* initialise PIC (for eg. keyboard input & timers */
	pic::init();
	// disable IRQ interrupts
	outb(0x21, 0xFF);
	outb(0xA1, 0xFF);

	/* initialise PIT */
	pit::init_pit0();
	pic::clear_mask(0);

	/* initialise PS/2 controller */
	ps2::init();
	pic::clear_mask(1);

	asm volatile("sti" ::: "memory");
}

/*
 * put the actual implementation outside of the extern "C" section, idk if I
 * can use c++ features inside the extern "C" section
 */
void kernel_main(void) {
	puts("Character set:");
	for (uint8_t high_half = 0; high_half < 0xF; ++high_half) {
		constexpr uint16_t indent = 0x2020;
		term_write_raw((uint8_t*)&indent, 2);
		for (uint8_t low_half = 0; low_half < 0xF; ++low_half) {
			term_putbyte(low_half + (high_half << 4));
		}
		putchar('\n');
	}

	const char *answer_seekers_computer_builders = "mouse people";

	printf("Hi there %s, the answer to your query is: %d\n", answer_seekers_computer_builders, 42);

	for (int i = 0; i < 1024 * 1024; ++i) io_wait();

	printf("Milliseconds since startup: %u\n", pit::millis);

	// copied from C code, TODO: clean this up
	for (;;) {
		while (!ps2::events.empty()) {
			ps2::Event event = ps2::events.pop();
			if (event.type != ps2::EventType::Press && event.type != ps2::EventType::Bounce) continue;

			if (event.key == ps2::KEY_BACKSPACE) {
				term_backspace();
			} else if (ps2::key_ascii_map[event.key]) {
				putchar(ps2::key_ascii_map[event.key]);
			}
		}
	}
}

extern "C" void __cxa_pure_virtual() {
	puts("failed creating pure virtual function!");
	abort();
}
