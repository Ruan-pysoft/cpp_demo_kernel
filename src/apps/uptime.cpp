#include "apps/uptime.hpp"

#include <stdint.h>
#include <stdio.h>

#include "pit.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace uptime {

namespace {

void draw() {
	const auto _ = term::Backbuffer();

	const uint32_t now = pit::millis;

	uint32_t seconds = now / 1000;
	uint32_t minutes = seconds / 60;
	seconds -= minutes * 60;
	uint32_t hours = minutes / 60;
	minutes -= hours * 60;

	term::clear();
	term::go_to(0, 0);

	printf("The system has been online for %u hours, %u minutes, and %u seconds.\n", hours, minutes, seconds);
	puts("Press Q or ESC to quit.");
}

}

void main() {
	for (;;) {
		draw();

		while (!ps2::events.empty()) {
			const auto e = ps2::events.pop();

			if (e.key == ps2::KEY_Q || e.key == ps2::KEY_ESCAPE) {
				return;
			}
		}

		__asm__ volatile("hlt" ::: "memory");
	}
}

}
