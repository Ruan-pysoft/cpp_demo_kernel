#include "idt.hpp"

#include <stdint.h>

#include <stdio.h>

#include "isr.hpp"
#include "gdt.hpp"

namespace idt {

namespace {

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

};

void init() {
	#define IDT_INT(addr) create_idt_entry(uint32_t(addr), gdt::kcode_segment, INTERRUPT_GATE_FLAGS)
	#define IDT_TRP(addr) create_idt_entry(uint32_t(addr), gdt::kcode_segment, TRAP_GATE_FLAGS)
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
}
void load() {
	asm volatile("lidt %[idtr]" :: [idtr] "m" (idt_descriptor) : "memory");
}

};
