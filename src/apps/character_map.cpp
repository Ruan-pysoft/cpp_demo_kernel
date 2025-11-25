#include "apps/character_map.hpp"

#include <stdint.h>
#include <string.h>

#include "ps2.hpp"
#include "vga.hpp"

using namespace term;

namespace character_map {

void main() {
	const char *title = "CHARACTER SET";
	const size_t title_len = strlen(title);
	const size_t title_offset = (vga::WIDTH - title_len)/2;

	go_to(title_offset, 1);
	setcolor(vga::entry_color(vga::Color::Black, vga::Color::LightGrey));
	writestring(title);
	resetcolor();

	const size_t line_width = 0x10;
	const size_t line_offset = (vga::WIDTH - line_width)/2;

	go_to(line_offset, 3);
	for (uint8_t low_half = 0; low_half <= 0xF; ++low_half) {
		if (low_half < 0xA) {
			putchar('0'+low_half);
		} else {
			putchar('a'+low_half-0xA);
		}
	}

	for (uint8_t high_half = 0; high_half <= 0xF; ++high_half) {
		go_to(line_offset - 3, 5 + high_half);

		if (high_half < 0xA) {
			putchar('0'+high_half);
		} else {
			putchar('a'+high_half-0xA);
		}
		putchar(' ');
		putchar(' ');

		for (uint8_t low_half = 0; low_half <= 0xF; ++low_half) {
			putbyte(low_half + (high_half << 4));
		}
	}

	const char *help_text = "Press ESC or Q to quit";
	const size_t help_len = strlen(help_text);
	const size_t help_offset = (vga::WIDTH - help_len)/2;

	go_to(help_offset, vga::HEIGHT-1 - 1);
	writestring(help_text);

	for (;;) {
		__asm__ volatile("hlt" ::: "memory");

		while (!ps2::events.empty()) {
			const auto event = ps2::events.pop();

			if (event.type == ps2::EventType::Press && (
				event.key == ps2::KEY_ESCAPE || event.key == ps2::KEY_Q
			)) {
				return;
			}
		}
	}
}

}
