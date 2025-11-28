/* from https://wiki.osdev.org/Bare_Bones */

#include <stddef.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>

#include "idt.hpp"
#include "vga.hpp"
#include "pic.hpp"
#include "pit.hpp"
#include "ps2.hpp"
#include "ioport.hpp"
#include "gdt.hpp"

#include "apps/main_menu.hpp"

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
	term::init();

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
	main_menu::main();
}

extern "C" void __cxa_pure_virtual() {
	puts("failed creating pure virtual function!");
	abort();
}
