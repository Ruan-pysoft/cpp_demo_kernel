#include "gdt.hpp"

#include "reload_segments.hpp"

namespace gdt {

namespace {

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
// only three gdt entries: NULL entry, code segment, and data segment
static uint64_t gdt[3];
static uint64_t gdt_descriptor = 0;

};

void init() {
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
}
void load() {
	asm volatile("lgdt (%[gdtr])" :: [gdtr] "m" (gdt_descriptor) : "memory");
	reloadSegments();
}

};
