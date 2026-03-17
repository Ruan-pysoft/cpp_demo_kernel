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

enum LastState {
	LS_HEAD,
	LS_TAIL,
	LS_NONE,
};

struct State {
	bool should_quit = false;

	sdk::random::Xorshift32 prng {};

	int curr_heads = 0;
	int curr_tails = 0;

	LastState last_state = LS_NONE;
	int curr_consec = 0;

	int longest_run = 0;
	int longest_heads = 0;
	int longest_tails = 0;
	int total_flipped = 0;

	int truncated_rounds = 0;
	int rounds_complete = 0;
	double avg_ratio = 0;
};

void handle_keypress(State &state, ps2::Event event) {
	if (event.type == ps2::EventType::Press && event.key == ps2::KEY_ESCAPE) {
		state.should_quit = true;
	}
}

void print_order(int n) {
	if (n < 1000) {
	} else if (n < 1000 * 1000) {
		printf("(%sthousands)",
			n < 10 * 1000 ? "" :
			n < 100 * 1000 ? "tens of " :
			"hundreds of "
		);
	} else if (n < 1000 * 1000 * 1000) {
		printf("(%smillions)",
			n < 10 * 1000 * 1000 ? "" :
			n < 100 * 1000 * 1000 ? "tens of " :
			"hundreds of "
		);
	} else {
		printf("(billions)");
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
	printf("Current no. of heads: %d ", state.curr_heads);
	print_order(state.curr_heads);
	term::go_to(3, 10);
	printf("Current no. of tails: %d ", state.curr_tails);
	print_order(state.curr_tails);
	term::go_to(3, 11);
	printf("Current heads/total: ");
	if (state.curr_heads+state.curr_tails) {
		print_double(state.curr_heads/(double)(state.curr_heads+state.curr_tails));
	} else putchar('/');

	term::go_to(1, 13);
	printf("Longest run so far: %d ", state.longest_run);
	print_order(state.longest_run);
	term::go_to(1, 14);
	printf("Longest run of heads so far: %d ", state.longest_heads);
	print_order(state.longest_heads);
	term::go_to(1, 15);
	printf("Longest run of tails so far: %d ", state.longest_tails);
	print_order(state.longest_tails);
	term::go_to(1, 16);
	printf("Total number of coins flipped so far: %d ", state.total_flipped);
	print_order(state.total_flipped);

	term::go_to(1, 18);
	printf("Number of truncated rounds: %d ", state.truncated_rounds);
	print_order(state.truncated_rounds);
	term::go_to(3, 19);
	printf("(Once 2^28 coin flips has been reached, the ratio is approximated as 0.5)");

	term::go_to(0, 0);
}

void tick(State &state) {
	uint32_t work_until = pit::millis + 1000/24;
	while (pit::millis < work_until) {
		if (state.prng.next()&1) {
			++state.curr_heads;
			if (state.last_state == LS_HEAD) {
				++state.curr_consec;
			} else {
				if (state.last_state == LS_TAIL && state.longest_tails < state.curr_consec) {
					state.longest_tails = state.curr_consec;
				}

				state.last_state = LS_HEAD;
				state.curr_consec = 1;
			}
		} else {
			++state.curr_tails;
			if (state.last_state == LS_TAIL) {
				++state.curr_consec;
			} else {
				if (state.last_state == LS_HEAD && state.longest_heads < state.curr_consec) {
					state.longest_heads = state.curr_consec;
				}

				state.last_state = LS_TAIL;
				state.curr_consec = 1;
			}
		}
		++state.total_flipped;

		if (state.curr_heads + state.curr_tails > state.longest_run) {
			state.longest_run = state.curr_heads + state.curr_tails;
		}

		if (state.curr_heads > state.curr_tails) {
			double ratio = state.curr_heads/(double)(state.curr_heads+state.curr_tails);
			state.avg_ratio = (state.avg_ratio*state.rounds_complete + ratio)/(state.rounds_complete+1);
			++state.rounds_complete;

			state.curr_heads = 0;
			state.curr_tails = 0;
		} else if (state.curr_heads + state.curr_tails >= 1<<28) {
			state.avg_ratio = (state.avg_ratio*state.rounds_complete + 0.5)/(state.rounds_complete+1);
			++state.rounds_complete;
			++state.truncated_rounds;

			state.curr_heads = 0;
			state.curr_tails = 0;
		}
		//pit::sleep(1000/24);
	}
}

State state{};

}

void main() {
	//State state{};
	state.should_quit = false;

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
