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

class Foo {
	const char *str;
public:
	Foo() : str("baz") {}
	~Foo() {
		printf("Bye!\n");
	}

	void bar() {
		term_writestring(str);
		term_writestring(": ");
	}
};

Foo g_foo;

class Static {
	int i;
public:
	Static(int i) : i(i) {}
	~Static() {}

	int geti() { return i; }
	void seti(int newi) { i = newi; }
	void inc() { ++i; }
};

class Dynamic {
	char *data;
	size_t size;
	// add some fields to hopefully prevent the new from not being called?
	bool b;
	int inum;
	float stuff[16];
public:
	Dynamic(const char *str) : size(strlen(str)), b(true), inum(42), stuff{0} {
		printf("Dynamic(\"%s\")\n", str);

		data = new char[size+1];
		memcpy(data, str, size+1);
	}
	~Dynamic() {
		printf("~Dynamic(\"%s\")\n", data);
		printf("%d %d %p\n", b, inum, stuff);

		delete[] data;
	}

	void poke() {
		for (size_t i = 0; i < size; ++i) {
			data[i] ^= 'a'^'A';
		}
	}
};

/*
 * put the actual implementation outside of the extern "C" section, idk if I
 * can use c++ features inside the extern "C" section
 */
void kernel_main(void) {
	Foo *p_foo = &g_foo;
	p_foo->bar();

	static Static s_var(13);

	s_var.inc();
	printf(":%d:", s_var.geti());

	puts("This first line will be scrolled offscreen");
	putchar('\n');
	putchar('\n');

	/* Newline support is left as an exercise. */
	puts("Hello, kernel world!");

	term_writestring("Kyk, ek kan selfs Afrikaans hier skryf! D" "\xA1" "e projek is ");
	term_setcolor(vga_entry_color(
		vga_color(uint8_t(vga_fgcolor(term_getcolor()))^8),
		vga_bgcolor(term_getcolor())
	));
	term_writestring("cool");
	term_resetcolor();
	term_writestring(" omdat:\n");

	puts(" - foo");
	puts(" - bar");
	puts(" - baz");
	puts("Lorum ipsum dolor set...");

	// /*
	putchar('\n');
	puts("Character set:");
	for (uint8_t high_half = 0; high_half < 0xF; ++high_half) {
		constexpr uint16_t indent = 0x2020;
		term_write_raw((uint8_t*)&indent, 2);
		for (uint8_t low_half = 0; low_half < 0xF; ++low_half) {
			term_putbyte(low_half + (high_half << 4));
		}
		putchar('\n');
	}
	// */

	const char *answer_seekers_computer_builders = "mouse people";

	printf("Hi there %s, the answer to your query is: %d\n", answer_seekers_computer_builders, 42);

	Dynamic stack_var("plates");
	Dynamic *heap_var = new Dynamic("Compost");

	stack_var.poke();
	heap_var->poke();

	delete heap_var;

	static Static *ds_var = new Static(4);

	ds_var->inc();
	printf(":%d:", ds_var->geti());

	printf("Milliseconds since startup: %u\n", pit::millis);

	// copied from C code, TODO: clean this up
	for (;;) {
		while (!ps2::events.empty()) {
			ps2::Event event = ps2::events.pop();
			if (event.type != ps2::EventType::Press && event.type != ps2::EventType::Bounce) continue;

			if (event.key == ps2::KEY_BACKSPACE) {
				BLT_WRITE_CHR('B');
				// TODO: implement backspace
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
