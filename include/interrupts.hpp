#pragma once

#include <stdint.h>

// see https://wiki.osdev.org/Interrupts_Tutorial

struct __attribute__((packed)) idt_entry_t {
	uint16_t isr_low; // lower 16 bits of ISR address
	uint16_t kernel_cs; // gdt segment selector that the cpu will load into cs before calling isr
};
