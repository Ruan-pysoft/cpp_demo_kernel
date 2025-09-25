/* from https://wiki.osdev.org/Bare_Bones */

#include <stddef.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vga.hpp"

/* Check if the compiler thinks you are targeting the wrong operating system */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with an ix86-elf compiler"
#endif

static inline uint64_t create_gdt_entry(uint32_t base, uint32_t limit, uint8_t flags, uint8_t access) {
	// from https://wiki.osdev.org/GDT_Tutorial
	// and https://wiki.osdev.org/Global_Descriptor_Table
	uint64_t res = 0;

	// first, set up the high 32 bits:

	res |= base  & 0xFF000000;        // base 24:31
	res |= uint32_t(flags&0xF) << 20; // flags 0:3
	res |= limit & 0x000F0000;        // limit 16:19
	res |= uint32_t(access) << 8;     // access 0:7
	res |= (base >> 16) & 0xFF;       // base 16:23

	res <<= 32;

	// next, set up the low 32 bits:

	res |= base << 16;         // base 0:15
	res |= limit & 0x0000FFFF; // limit 0:15

	return res;
}

/*
 * make the kernel_main function extern "C"
 * bc idk what funky stuff C++ does with calling conventions,
 * this way I know how to call it from assembly
 */
/* also, to avoid name mangling */
extern "C" void kernel_early_main(void);
extern "C" void kernel_main(void);

static uint64_t gdt[3];
static uint64_t gdt_descriptor = 0;
void kernel_early_main() {
	/* Initialize terminal interface */
	term_init();

	// set up the GDT
	// see https://wiki.osdev.org/GDT_Tutorial
	// and https://wiki.osdev.org/Global_Descriptor_Table

	// flat setup, only kernel (ring 0)
	const uint64_t gdt_null = create_gdt_entry(0, 0, 0, 0);
	const uint64_t gdt_kernel_code = create_gdt_entry(0, 0xFFFFF, 0xC, 0x9A);
	const uint64_t gdt_kernel_data = create_gdt_entry(0, 0xFFFFF, 0xC, 0x92);

	gdt[0] = gdt_null;
	gdt[1] = gdt_kernel_code;
	gdt[2] = gdt_kernel_data;

	const uint16_t gdt_size = sizeof(gdt)-1;
	const uint32_t gdt_addr = uint32_t(gdt);

	gdt_descriptor = gdt_addr;
	gdt_descriptor <<= 16;
	gdt_descriptor |= gdt_size;

	asm volatile("lgdt %[gdtr]" :: [gdtr] "m" (gdt_descriptor) : "memory");
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
}

extern "C" void __cxa_pure_virtual() {
	puts("failed creating pure virtual function!");
	abort();
}
