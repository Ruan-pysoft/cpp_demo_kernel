#include "pit.hpp"

#include "ioport.hpp"

static volatile uint32_t sleep_counter = 0;
static bool course = false;

void pit_handle_trigger() {
	if (!course) {
		++pit::millis;
		if (sleep_counter > 0) --sleep_counter;
	} else {
		pit::millis += 16;
		if (sleep_counter > 0) {
			if (sleep_counter <= 16) sleep_counter = 0;
			else sleep_counter -= 16;
		}
	}
}

namespace pit {
	volatile uint64_t millis = 0;

	void init_pit0() {
		asm volatile("cli" ::: "memory");
		// configure, but don't enable interrupts
		outb(PIT_COMM,
			PIT_COMM_CHAN0 | PIT_COMM_ACCESS_LOBYTE
			| PIT_COMM_ACCESS_HIBYTE | PIT_COMM_MODE2
		);
		// set pit count, but don't enable interrupts
		constexpr uint16_t count = 1193;
		outb(PIT_CHAN0_DATA, count&0xFF);
		outb(PIT_CHAN0_DATA, count>>8);
	}

	void set_pit_count(uint16_t count) {
		__asm__ volatile("cli" ::: "memory");

		outb(PIT_CHAN0_DATA, count&0xFF);
		outb(PIT_CHAN0_DATA, count>>8);

		__asm__ volatile("sti" ::: "memory");
	}
	void configure(uint8_t flags) {
		__asm__ volatile("cli" ::: "memory");
		outb(PIT_COMM, flags);
		__asm__ volatile("sti" ::: "memory");
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
	void sleep_coarse(uint32_t millis) {
		sleep_counter = millis;

		set_pit_count(19091);
		course = true;

		while (sleep_counter) {
			asm volatile("hlt");
		}

		course = false;
		set_pit_count(1193);
	}
}
