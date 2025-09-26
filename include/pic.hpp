#pragma once

#include <stdint.h>

/* see https://wiki.osdev.org/8259_PIC */

#define PIC1 0x20
#define PIC2 0xA0

#define PIC1_COMM PIC1
#define PIC1_DATA (PIC1+1)

#define PIC2_COMM PIC2
#define PIC2_DATA (PIC2+1)

#define PIC_EOI 0x20 /* end of interrupt code */
namespace pic {
	void send_eoi(uint8_t irq);

	void init();

	void set_mask(uint8_t irq_line);
	void clear_mask(uint8_t irq_line);
}
