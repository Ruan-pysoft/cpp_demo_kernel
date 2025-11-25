#include "apps/callback_demo.hpp"

#include <stdio.h>

#include "blit.hpp"
#include "eventloop.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace callback_demo {

namespace {

struct State {
	bool should_quit = false;
};

void keyevent_callback(State *state, ps2::Event event) {
	if (event.type != ps2::EventType::Press && event.type != ps2::EventType::Bounce) return;

	if (event.key == ps2::KEY_ESCAPE) {
		state->should_quit = true;
	} else if (event.key == ps2::KEY_BACKSPACE) {
		term::backspace();
	} else if (ps2::key_ascii_map[event.key]) {
		putchar(ps2::key_ascii_map[event.key]);
	}
}

};

void main() {
	State state{};

	CallbackEventLoop event_loop {
		(CallbackEventLoop::cb_t)keyevent_callback,
		&state
	};

	puts("Demo using callback-based event loop");
	puts("Press Esc to quit");

	while (!state.should_quit) { // write a period to the debug section every two seconds
		auto _ = event_loop.get_frame(2'000);
		BLT_WRITE_CHR('.');
	}

	putchar('\n');
	puts("Done!");
}

}
