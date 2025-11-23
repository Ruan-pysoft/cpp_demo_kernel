/* from https://wiki.osdev.org/Bare_Bones */

#include <stddef.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vga.hpp"
#include "isr.hpp"
#include "reload_segments.hpp"
#include "pic.hpp"
#include "pit.hpp"
#include "ps2.hpp"
#include "blit.hpp"
#include "ioport.hpp"

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
static uint64_t gdt[3];
/*static struct __attribute__((packed)) {
	uint16_t size;
	uint32_t addr;
} gdt_descriptor;*/
static uint64_t gdt_descriptor = 0;

static constexpr uint16_t create_segment_selector(uint16_t index, bool use_local, uint8_t privilege_level) {
	return (index<<3) | uint16_t(use_local<<2) | (privilege_level&0b11);
}
static constexpr uint16_t kcode_segment = create_segment_selector(1, false, 0);
static constexpr uint16_t kdata_segment = create_segment_selector(2, false, 0);

#define IDT_GATE_TASK        0b0101
#define IDT_GATE_INTERRUPT16 0b0110
#define IDT_GATE_TRAP16      0b0111
#define IDT_GATE_INTERRUPT32 0b1110
#define IDT_GATE_TRAP32      0b1111
#define IDT_DPL(ring)        ((ring&0b11)<<5)
#define IDT_FLAG_PRESENT     0b1000'0000
#define IDT_FLAG_ZERO        0b0001'0000
static inline uint64_t create_idt_entry(uint32_t offset, uint16_t segment, uint8_t flags) {
	flags |= IDT_FLAG_PRESENT;
	flags &= ~IDT_FLAG_ZERO;

	// see https://wiki.osdev.org/Interrupt_Descriptor_Table
	uint64_t res = 0;

	// first, set up the high 32 bits:

	res |= offset & 0xFFFF0000; // offset 16:31
	res |= uint32_t(flags)<<8;  // flags
	res &= ~0xFF;               // zero the reserved bits

	res <<= 32;

	// next, set up the low 32 bits:

	res |= uint32_t(segment) << 16; // segment
	res |= offset&0xFFFF;           // offset 0:15

	return res;
}
constexpr uint8_t INTERRUPT_GATE_FLAGS = IDT_GATE_INTERRUPT32 | IDT_DPL(0) | IDT_FLAG_PRESENT;
constexpr uint8_t TRAP_GATE_FLAGS = IDT_GATE_TRAP32 | IDT_DPL(0) | IDT_FLAG_PRESENT;
static uint64_t idt[256] = {0};
static uint64_t idt_descriptor = 0;

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

	asm volatile("lgdt (%[gdtr])" :: [gdtr] "m" (gdt_descriptor) : "memory");
	reloadSegments();

	printf("isr0x00_DE addr: %p\n", isr0x00_DE);

	#define IDT_INT(addr) create_idt_entry(uint32_t(addr), kcode_segment, INTERRUPT_GATE_FLAGS)
	#define IDT_TRP(addr) create_idt_entry(uint32_t(addr), kcode_segment, TRAP_GATE_FLAGS)
	#define IDT_FLT(addr) IDT_TRP(addr)
	idt[0x00] = IDT_FLT(isr0x00_DE);
	idt[0x01] = IDT_TRP(isr0x01_DB);
	idt[0x02] = IDT_INT(isr0x02_NMI);
	idt[0x03] = IDT_TRP(isr0x03_BP);
	idt[0x04] = IDT_TRP(isr0x04_OF);
	idt[0x05] = IDT_FLT(isr0x05_BR);
	idt[0x06] = IDT_FLT(isr0x06_UD);
	idt[0x07] = IDT_FLT(isr0x07_NM);
	idt[0x08] = IDT_INT(isr0x08_DF); // ABORT
	idt[0x0A] = IDT_FLT(isr0x0A_TS);
	idt[0x0B] = IDT_FLT(isr0x0B_NP);
	idt[0x0C] = IDT_FLT(isr0x0C_SS);
	idt[0x0D] = IDT_FLT(isr0x0D_GP);
	idt[0x0E] = IDT_FLT(isr0x0E_PF);

	idt[0x0F] = IDT_INT(isr0x0F_INTEL); // intel reserved

	idt[0x10] = IDT_FLT(isr0x10_MF);
	idt[0x11] = IDT_FLT(isr0x11_AC);
	idt[0x12] = IDT_INT(isr0x12_MC); // ABORT
	idt[0x13] = IDT_FLT(isr0x13_XM);
	idt[0x14] = IDT_FLT(isr0x14_VE);
	idt[0x15] = IDT_FLT(isr0x15_CP);

	idt[0x16] = IDT_FLT(isr0x16_CPU);
	idt[0x17] = IDT_FLT(isr0x17_CPU);
	idt[0x18] = IDT_FLT(isr0x18_CPU);
	idt[0x19] = IDT_FLT(isr0x19_CPU);
	idt[0x1A] = IDT_FLT(isr0x1A_CPU);
	idt[0x1B] = IDT_FLT(isr0x1B_CPU);
	idt[0x1C] = IDT_FLT(isr0x1C_CPU);
	idt[0x1D] = IDT_FLT(isr0x1D_CPU);
	idt[0x1E] = IDT_FLT(isr0x1E_CPU);
	idt[0x1F] = IDT_FLT(isr0x1F_CPU);

	idt[0x20] = IDT_FLT(isr0x20_IRQ);
	idt[0x21] = IDT_FLT(isr0x21_IRQ);
	idt[0x22] = IDT_FLT(isr0x22_IRQ);
	idt[0x23] = IDT_FLT(isr0x23_IRQ);
	idt[0x24] = IDT_FLT(isr0x24_IRQ);
	idt[0x25] = IDT_FLT(isr0x25_IRQ);
	idt[0x26] = IDT_FLT(isr0x26_IRQ);
	idt[0x27] = IDT_FLT(isr0x27_IRQ);
	idt[0x28] = IDT_FLT(isr0x28_IRQ);
	idt[0x29] = IDT_FLT(isr0x29_IRQ);
	idt[0x2A] = IDT_FLT(isr0x2A_IRQ);
	idt[0x2B] = IDT_FLT(isr0x2B_IRQ);
	idt[0x2C] = IDT_FLT(isr0x2C_IRQ);
	idt[0x2D] = IDT_FLT(isr0x2D_IRQ);
	idt[0x2E] = IDT_FLT(isr0x2E_IRQ);
	idt[0x2F] = IDT_FLT(isr0x2F_IRQ);
	#undef IDT_FLT
	#undef IDT_TRP
	#undef IDT_INT

	const uint16_t idt_size = sizeof(*idt)*0x2F;
	const uint32_t idt_addr = uint32_t(idt);

	idt_descriptor = idt_addr;
	idt_descriptor <<= 16;
	idt_descriptor |= idt_size;

	printf("idt_size: %p\n", uint32_t(idt_size));
	printf("idt_addr: %p\n", idt_addr);
	printf("idtr:     %p %p\n", uint32_t(idt_descriptor >> 32), uint32_t(idt_descriptor));

	asm volatile("lidt %[idtr]" :: [idtr] "m" (idt_descriptor) : "memory");

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
