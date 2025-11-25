#include "apps/ignore_demo.hpp"

#include <stdio.h>
#include <string.h>

#include "eventloop.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace ignore_demo {

namespace {

void atomic_read_state(bool into[ps2::KEY_MAX]) {
	__asm__ volatile("cli" ::: "memory");
	memcpy(into, ps2::key_state, sizeof(bool)*(size_t)ps2::KEY_MAX);
	__asm__ volatile("sti" ::: "memory");
}

}

void main() {
	IgnoreEventLoop event_loop{};

	bool should_quit = false;
	bool back_state[ps2::KEY_MAX];
	atomic_read_state(back_state);

	puts("Demo using eventloop ignoring keyboard events");
	puts("Press Esc to quit");

	while (!should_quit) {
		auto _ = event_loop.get_frame(17); // ~ 60 fps

		for (ps2::Key key = (ps2::Key)0; key < ps2::KEY_MAX; key = (ps2::Key)((int)key+1)) {
			if (ps2::key_state[key] && !back_state[key]) {
				// key has been pressed

				if (key == ps2::KEY_ESCAPE) {
					should_quit = true;
				} else if (key == ps2::KEY_BACKSPACE) {
					term::backspace();
				} else if (ps2::key_ascii_map[key]) {
					putchar(ps2::key_ascii_map[key]);
				}
			}
		}

		atomic_read_state(back_state);
	}

	putchar('\n');
	puts("Done!");
}

}
