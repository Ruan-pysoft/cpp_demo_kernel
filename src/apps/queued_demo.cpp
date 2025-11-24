#include "apps/queued_demo.hpp"

#include <stdio.h>

#include "eventloop.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace queued_demo {

void main() {
	QueuedEventLoop event_loop{};

	bool should_quit = false;

	puts("Demo using queued event loop");
	puts("Press Esc to quit");

	while (!should_quit) {
		auto _ = event_loop.get_frame(17); // 60 fps

		for (auto event : event_loop.events()) {
			if (event.type != ps2::EventType::Press && event.type != ps2::EventType::Bounce) continue;

			if (event.key == ps2::KEY_ESCAPE) {
				should_quit = true;
			} else if (event.key == ps2::KEY_BACKSPACE) {
				term_backspace();
			} else if (ps2::key_ascii_map[event.key]) {
				putchar(ps2::key_ascii_map[event.key]);
			}
		}
	}

	putchar('\n');
	puts("Done!");
}

}
