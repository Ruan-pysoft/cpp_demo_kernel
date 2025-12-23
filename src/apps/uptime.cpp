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

	uint32_t millis = now;
	uint32_t seconds = now / 1000;
	millis -= seconds * 1000;
	uint32_t minutes = seconds / 60;
	seconds -= minutes * 60;
	uint32_t hours = minutes / 60;
	minutes -= hours * 60;
	uint32_t days = hours / 24;
	hours -= days * 24;

	term::clear();
	term::go_to(0, 0);

	char time_display[] = "HH:MM:SS.MMM";

	time_display[0] = '0' + hours/10;
	time_display[1] = '0' + hours%10;

	time_display[3] = '0' + minutes/10;
	time_display[4] = '0' + minutes%10;

	time_display[6] = '0' + seconds/10;
	time_display[7] = '0' + seconds%10;

	time_display[9] = '0' + millis/100;
	time_display[10] = '0' + (millis/10)%10;
	time_display[11] = '0' + millis%10;

	if (days) {
		printf("System Uptime: %u days, %s\n", days, time_display);
	} else {
		printf("System Uptime: %s\n", time_display);
	}
	puts("Press Q or ESC to quit.");
}

}

void main() {
	// TEST: pit::millis += 542867211;

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
