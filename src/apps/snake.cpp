#include "apps/snake.hpp"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "eventloop.hpp"
#include "pit.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace snake {

namespace {

enum class Direction {
	Left,
	Down,
	Up,
	Right,
};

class Prng {
	// see https://en.wikipedia.org/wiki/Xorshift
	uint32_t state;
public:
	Prng() : Prng(pit::millis) {}
	// XOR'ing seed with 0b10101010.... to get a good distribution of active bits initially
	Prng(uint32_t seed) : state(seed ^ 0xAA'AA'AA'AA) {
		if (seed == 0) {
			// seed shouldn't be zero!
			seed = pit::millis; // assume that pit::millis is never zero
		}
	}

	uint32_t next() {
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		return state;
	}
};

struct Pos {
	uint8_t x, y;

	bool operator==(const struct Pos &other) const {
		return x == other.x && y == other.y;
	}

	static Pos random_pos(Prng &prng) {
		const uint32_t rand = prng.next();
		// technically biased towards lower numbers, but who cares?
		return {
			.x = (uint8_t)((rand&0xFF) % VGA_WIDTH),
			.y = (uint8_t)(((rand>>8)&0xFF) % VGA_HEIGHT),
		};
	}
};

struct State;
constexpr size_t MAX_SNAKE_SIZE = VGA_WIDTH*VGA_HEIGHT;
class Snake {
	Pos segments[MAX_SNAKE_SIZE];
	size_t num_segments;

	Direction prev_facing;
	bool has_prev_facing = false;
	Direction facing;
public:
	Snake(Pos start = {VGA_WIDTH/2, VGA_HEIGHT/2}, size_t initial_segments = 5, Direction initial_dir = Direction::Left)
	: num_segments(initial_segments), facing(initial_dir) {
		for (size_t i = 0; i < num_segments; ++i) {
			segments[i] = start;
		}
	}

	Direction get_facing() const {
		return facing;
	}
	void look(Direction dir) {
		if (has_prev_facing) {
			if (prev_facing == Direction::Left && dir == Direction::Right) return;
			if (prev_facing == Direction::Down && dir == Direction::Up) return;
			if (prev_facing == Direction::Up && dir == Direction::Down) return;
			if (prev_facing == Direction::Right && dir == Direction::Left) return;
		}
		facing = dir;
	}

	void update(State &state);
	void draw() const {
		size_t i = num_segments;
		while (i --> 0) {
			term_goto(segments[i].x, segments[i].y);
			term_setcolor(vga_entry_color(
				vga_color::green,
				vga_color::black
			));
			term_putchar('@');
			term_resetcolor();
		}
	}
};

struct State {
	bool should_quit = false;
	bool restart = false;
	bool lost = false;
	bool blink_state = false;
	Prng prng{}; // I assume the constructor gets called each tame State is created? idk, I'll figure it out later
	Snake snake{};
	Pos apple;
	int score = 0;
};

void handle_keypress(State *state, ps2::Event event) {
	if (event.type == ps2::EventType::Press) {
		switch (event.key) {
			case ps2::KEY_Q:
			case ps2::KEY_ESCAPE: {
				state->should_quit = true;
			} break;
			case ps2::KEY_R:
			case ps2::KEY_ENTER: {
				// only allow the player to restart from the game over screen;
				// if they want to restart in play, they just have to run into themselves
				if (state->lost) state->restart = true;
			} break;

			case ps2::KEY_LEFT:
			case ps2::KEY_A:
			case ps2::KEY_H: {
				state->snake.look(Direction::Left);
			} break;
			case ps2::KEY_DOWN:
			case ps2::KEY_D:
			case ps2::KEY_J: {
				state->snake.look(Direction::Down);
			} break;
			case ps2::KEY_UP:
			case ps2::KEY_W:
			case ps2::KEY_K: {
				state->snake.look(Direction::Up);
			} break;
			case ps2::KEY_RIGHT:
			case ps2::KEY_S:
			case ps2::KEY_L: {
				state->snake.look(Direction::Right);
			} break;
			default: break;
		}
	}
}

void draw(State &state) {
	term_clear();
	cursor_disable();

	term_goto(2, 1);
	printf("Score: %d", state.score);

	term_goto(state.apple.x, state.apple.y);
	term_setcolor(vga_entry_color(
		vga_color::red,
		vga_color::black
	));
	term_putchar('a');
	term_resetcolor();

	state.snake.draw();

	if (state.lost) {
		const char *lose_text =  " ** GAME OVER ** ";
		const char *frame_text = "                 ";
		const size_t lose_text_len = strlen(lose_text);
		const size_t lose_text_offset = (VGA_WIDTH - lose_text_len)/2;

		if (state.blink_state) {
			term_setcolor(vga_entry_color(
				vga_color::white,
				vga_color::red
			));
		} else {
			term_setcolor(vga_entry_color(
				vga_color::red,
				vga_color::white
			));
		}

		term_goto(lose_text_offset, VGA_HEIGHT/2 - 1);
		term_writestring(frame_text);
		term_goto(lose_text_offset, VGA_HEIGHT/2);
		term_writestring(lose_text);
		term_goto(lose_text_offset, VGA_HEIGHT/2 + 1);
		term_writestring(frame_text);

		term_resetcolor();
	}
}

void update(State &state) {
	if (!state.lost) state.snake.update(state);
	state.blink_state = !state.blink_state;
}

void Snake::update(State &state) {
	has_prev_facing = true;
	prev_facing = facing;

	Pos next_head_pos = segments[0];
	switch (facing) {
		case Direction::Left: {
			next_head_pos.x = next_head_pos.x == 0
				? VGA_WIDTH - 1 : next_head_pos.x - 1;
		} break;
		case Direction::Down: {
			next_head_pos.y = next_head_pos.y == VGA_HEIGHT - 1
				? 0 : next_head_pos.y + 1;
		} break;
		case Direction::Up: {
			next_head_pos.y = next_head_pos.y == 0
				? VGA_HEIGHT - 1 : next_head_pos.y - 1;
		} break;
		case Direction::Right: {
			next_head_pos.x = next_head_pos.x == VGA_WIDTH - 1
				? 0 : next_head_pos.x + 1;
		} break;
	}

	if (next_head_pos == state.apple) {
		state.apple = Pos::random_pos(state.prng);
		++state.score;
		if (num_segments < MAX_SNAKE_SIZE) {
			const Pos tail = segments[num_segments-1];
			segments[num_segments++] = tail;
		}
	}

	for (size_t i = 1; i < num_segments - 1; ++i) {
		// only check up to second-to-last segment,
		// as all segments are about to move forward
		// (so the head may move into the space which the tail currently occupies, but won't the next frame)
		if (next_head_pos == segments[i]) {
			state.lost = true;
			return;
		}
	}

	size_t i = num_segments;
	while (i --> 1) {
		segments[i] = segments[i-1];
	}
	segments[0] = next_head_pos;
}

}

void main() {
	bool should_quit = false;
	while (!should_quit) {
		State state{};
		state.apple = Pos::random_pos(state.prng);

		CallbackEventLoop event_loop{
			(CallbackEventLoop::cb_t)handle_keypress,
			&state,
		};

		draw(state);

		constexpr uint32_t starting_speed = 500; // 2 fps
		constexpr uint32_t ending_speed = 100; // 10 fps
		constexpr int by_score = 16;

		bool skip_frame = true;

		while (!state.should_quit && !state.restart) {
			uint32_t frame_time = starting_speed;
			frame_time -= (starting_speed - ending_speed) * state.score / by_score;
			frame_time = state.score >= by_score ? ending_speed : frame_time;
			frame_time = state.lost ? starting_speed : frame_time;
			auto _ = event_loop.get_frame(frame_time/2);

			bool sprint = ps2::key_state[ps2::KEY_LSHIFT] || ps2::key_state[ps2::KEY_RSHIFT]
				|| ps2::key_state[ps2::KEY_LCTL] || ps2::key_state[ps2::KEY_RCTL]
				|| ps2::key_state[ps2::KEY_SPACE];

			if (!skip_frame || state.lost || sprint) {
				update(state);
				draw(state);
			}

			skip_frame = !skip_frame;
		}

		should_quit = state.should_quit;
	}
}

}
