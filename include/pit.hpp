#pragma once

// https://wiki.osdev.org/Programmable_Interval_Timer

#include <stdint.h>

#define PIT_CHAN0_DATA 0x40
#define PIT_CHAN1_DATA 0x41
#define PIT_CHAN2_DATA 0x42
#define PIT_COMM 0x43

#define PIT_COMM_CHAN0 (0b00 << 6)
#define PIT_COMM_CHAN1 (0b01 << 6)
#define PIT_COMM_CHAN2 (0b10 << 6)

#define PIT_COMM_ACCESS_COUNT_VAL (0b00 << 4)
#define PIT_COMM_ACCESS_LOBYTE (0b01 << 4)
#define PIT_COMM_ACCESS_HIBYTE (0b10 << 4)

#define PIT_COMM_MODE0 (0b000 << 1) // interrupt on terminal count
#define PIT_COMM_MODE1 (0b001 << 1) // hardware re-triggerable one-shot
#define PIT_COMM_MODE2 (0b010 << 1) // rate generator
#define PIT_COMM_MODE3 (0b011 << 1) // square wave generator
#define PIT_COMM_MODE4 (0b100 << 1) // software triggered strobe
#define PIT_COMM_MODE5 (0b101 << 1) // hardware triggered strobe

#define PIT_COMM_BCD 0b1

extern "C" void pit_handle_trigger(void);

namespace pit {
	volatile extern uint32_t millis;

	void init_pit0(); // initialise channel 0 to cause an IRQ0 every millisecond

	void set_pit_count(uint16_t count);
	void configure(uint8_t flags);

	void sleep(uint32_t millis);
	void sleep_until(uint32_t system_time);
}
