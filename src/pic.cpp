#include "pic.hpp"

#include <stdint.h>

#include <ioport.hpp>

namespace pic {
	void send_eoi(uint8_t irq) {
		if (irq >= 8) outb(PIC2_COMM, PIC_EOI);
		outb(PIC1_COMM, PIC_EOI);
	}

#define ICW1_ICW4 0x01
#define ICW1_SINGLE 0x02
#define ICW1_INTERVAL 0x04
#define ICW1_LEVEL 0x08
#define ICW1_INIT 0x10

#define ICW4_8086 0x01
#define ICW4_AUTO 0x02
#define ICW4_BUF_SLAVE 0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM 0x10

#define CASCADE_IRQ 2

	void init() {
		const uint8_t offset1 = 0x20;
		const uint8_t offset2 = 0x28;

		outb(PIC1_COMM, ICW1_INIT | ICW1_ICW4); // start init sequence
		io_wait();
		outb(PIC2_COMM, ICW1_INIT | ICW1_ICW4);
		io_wait();

		outb(PIC1_DATA, offset1); // ICW2: Master PIC vector offset
		io_wait();
		outb(PIC2_DATA, offset2); // ICW2: Slave PIC vector offset
		io_wait();

		outb(PIC1_DATA, 1 << CASCADE_IRQ); // ICW3: tell Master PIC there is slave at IRQ2
		io_wait();
		outb(PIC2_DATA, CASCADE_IRQ); // ICW3: tell Master PIC its cascade identity (0000 0010)
		io_wait();

		outb(PIC1_DATA, ICW4_8086); // ICW4: let the PICs use 8086 mode (not 8080 mode)
		io_wait();
		outb(PIC2_DATA, ICW4_8086);
		io_wait();

		// unmask PICs
		outb(PIC1_DATA, 0);
		outb(PIC2_DATA, 0);
	}

	void set_mask(uint8_t irq_line) {
		uint16_t port;
		uint8_t value;

		if (irq_line < 8) {
			port = PIC1_DATA;
		} else {
			port = PIC2_DATA;
			irq_line -= 8;
		}

		value = inb(port) | (1 << irq_line);
		outb(port, value);
	}
	void clear_mask(uint8_t irq_line) {
		uint16_t port;
		uint8_t value;

		if (irq_line < 8) {
			port = PIC1_DATA;
		} else {
			port = PIC2_DATA;
			irq_line -= 8;
		}

		value = inb(port) & ~(1 << irq_line);
		outb(port, value);
	}
}
