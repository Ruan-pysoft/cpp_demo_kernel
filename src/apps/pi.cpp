#include "apps/pi.hpp"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <sdk/eventloop.hpp>
#include <sdk/random.hpp>
#include <sdk/terminal.hpp>

#include "pit.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace ps2 { extern bool key_event_pending; }

namespace pi {

namespace {

constexpr uint32_t SEED = 42;

struct State {
	bool should_quit = false;

	sdk::random::Xorshift32 prng {};

	int curr_heads = 0;
	int curr_tails = 0;

	int rounds_complete = 0;
	double avg_ratio = 0;
};

void handle_keypress(State &state, ps2::Event event) {
	if (event.type == ps2::EventType::Press && event.key == ps2::KEY_ESCAPE) {
		state.should_quit = true;
	}
}

void print_double(double d) {
	printf("%d.", (long long)d);
	d -= (long long)d;
	for (int i = 0; i < 6; ++i) {
		d *= 10;
		putchar('0' + (int)d);
		d -= (int)d;
	}
}

void draw(State &state) {
	auto _ = term::Backbuffer();

	term::clear();

	const char *title = "CALCULATING PI...";
	const size_t title_len = strlen(title);
	const size_t title_offset = (vga::WIDTH - title_len)/2;
	term::go_to(title_offset, 1);
	sdk::colors::with(
		vga::Color::DarkGrey, vga::Color::White,
		term::writestring, title
	);

	term::go_to(1, 3);
	printf("No of rounds completed: %d", state.rounds_complete);
	if (state.rounds_complete) {
		term::go_to(1, 5);
		printf("Current avg. ratio of heads/total flips: ");
		print_double(state.avg_ratio);
		term::go_to(3, 6);
		printf("Current estimated value of pi: ");
		print_double(state.avg_ratio*4);
	} else {
		term::go_to(1, 5);
		printf("Current avg. ratio of heads/total flips: -");
		term::go_to(3, 6);
		printf("Current estimated value of pi: -");
	}

	term::go_to(1, 8);
	printf("Current run:");
	term::go_to(3, 9);
	printf("Current no. of heads: %d", state.curr_heads);
	term::go_to(3, 10);
	printf("Current no. of tails: %d", state.curr_tails);
	term::go_to(3, 11);
	printf("Current heads/total: ");
	if (state.curr_heads+state.curr_tails) {
		print_double(state.curr_heads/(double)(state.curr_heads+state.curr_tails));
	} else putchar('/');

	term::go_to(0, 0);
}

void tick(State &state) {
	uint32_t work_until = pit::millis + 1000/24;
	while (pit::millis < work_until) {
		if (state.prng.next()&1) {
			++state.curr_heads;
		} else {
			++state.curr_tails;
		}

		if (state.curr_heads > state.curr_tails) {
			double ratio = state.curr_heads/(double)(state.curr_heads+state.curr_tails);
			state.avg_ratio = (state.avg_ratio*state.rounds_complete + ratio)/(state.rounds_complete+1);
			++state.rounds_complete;

			state.curr_heads = 0;
			state.curr_tails = 0;
		}
		//pit::sleep(1000/24);
	}
}

}

void main() {
	State state{};

	draw(state);

	while (!state.should_quit) {
		tick(state);

		if (ps2::key_event_pending) {
			while (!ps2::events.empty()) {
				handle_keypress(state, ps2::events.pop());
			}

			ps2::key_event_pending = false;
		}

		draw(state);
	}
}

}
