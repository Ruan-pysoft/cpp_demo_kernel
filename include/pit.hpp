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

	// sleep functions with callback will trigger the callback every time an interrupt is triggered
	void sleep(uint32_t millis);
	template<bool only_on_kb_events = false> // yes, this template is cursed... it should probably be a default parameter instead. But this way the code for callbacks on keyboard events only and on all interrupts are actually different functions, eliminating a (very, very minor) inefficiency
	// also, I only now realised I (seemingly?) cannot specify the name of the template parameter while supplying it, soo... probably should move it to a kw argument
	void sleep(uint32_t millis, void(*cb)(void*), void *cb_arg, bool cb_on_final = true);
	void sleep_until(uint32_t system_time);
	template<bool only_on_kb_events = false>
	void sleep_until(uint32_t system_time, void(*cb)(void*), void *cb_arg, bool cb_on_final = true);
	void sleep_coarse(uint32_t millis);
	template<bool only_on_kb_events = false>
	void sleep_coarse(uint32_t millis, void(*cb)(void*), void *cb_arg, bool cb_on_final = true);
}
