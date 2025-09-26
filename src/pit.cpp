#include "pit.hpp"

#include "ioport.hpp"

static volatile uint32_t sleep_counter = 0;

void pit_handle_trigger() {
	++pit::millis;
	if (sleep_counter > 0) --sleep_counter;
}

namespace pit {
	volatile uint32_t millis = 0;

	void init_pit0() {
		configure(
			PIT_COMM_CHAN0 | PIT_COMM_ACCESS_LOBYTE
			| PIT_COMM_ACCESS_HIBYTE | PIT_COMM_MODE2
		);
		set_pit_count(1193);
	}

	void set_pit_count(uint16_t count) {
		asm volatile("cli");

		outb(PIT_CHAN0_DATA, count&0xFF);
		outb(PIT_CHAN0_DATA, count>>8);

		asm volatile("sti");
	}
	void configure(uint8_t flags) {
		asm volatile("cli");

		outb(PIT_COMM, flags);

		asm volatile("sti");
	}

	void sleep(uint32_t millis) {
		sleep_counter = millis;

		while (sleep_counter) {
			asm volatile("hlt");
		}
	}
	void sleep_until(uint32_t system_time) {
		while (pit::millis < system_time) {
			asm volatile("hlt");
		}
	}
}
