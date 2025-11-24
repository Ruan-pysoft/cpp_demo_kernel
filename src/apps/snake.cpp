#include "apps/snake.hpp"

#include <stdint.h>

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
	static Pos random_pos(Prng &prng) {
		const uint32_t rand = prng.next();
		// technically biased towards lower numbers, but who cares?
		return {
			.x = (uint8_t)((rand&0xFF) % VGA_WIDTH),
			.y = (uint8_t)(((rand>>8)&0xFF) % VGA_HEIGHT),
		};
	}
};

constexpr size_t MAX_SNAKE_SIZE = VGA_WIDTH*VGA_HEIGHT;
class Snake {
	Pos segments[MAX_SNAKE_SIZE];
	size_t num_segments;
public:
	Direction facing;

	Snake(Pos start = {VGA_WIDTH/2, VGA_HEIGHT/2}, size_t initial_segments = 5, Direction initial_dir = Direction::Left)
	: num_segments(initial_segments), facing(initial_dir) {
		for (size_t i = 0; i < num_segments; ++i) {
			segments[i] = start;
		}
	}

	void move() {
		size_t i = num_segments;
		while (i --> 1) {
			segments[i] = segments[i-1];
		}
		switch (facing) {
			case Direction::Left: {
				if (segments[0].x == 0) segments[0].x = VGA_WIDTH - 1;
				else --segments[0].x;
			} break;
			case Direction::Down: {
				if (segments[0].y == VGA_HEIGHT - 1) segments[0].y = 0;
				else ++segments[0].y;
			} break;
			case Direction::Up: {
				if (segments[0].y == 0) segments[0].y = VGA_HEIGHT - 1;
				else --segments[0].y;
			} break;
			case Direction::Right: {
				if (segments[0].x == VGA_WIDTH - 1) segments[0].x = 0;
				else ++segments[0].x;
			} break;
		}
	}

	void draw() const {
		size_t i = num_segments;
		while (i --> 0) {
			term_goto(segments[i].x, segments[i].y);
			term_putchar('@');
		}
	}
};

struct State {
	bool should_quit = false;
	Prng prng{}; // I assume the constructor gets called each tame State is created? idk, I'll figure it out later
	Snake snake{};
	Pos apple;
};

void handle_keypress(State *state, ps2::Event event) {
	if (event.type == ps2::EventType::Press && (
		event.key == ps2::KEY_Q || event.key == ps2::KEY_ESCAPE
	)) {
		state->should_quit = true;
	}

	if (event.type == ps2::EventType::Press) {
		switch (event.key) {
			case ps2::KEY_LEFT:
			case ps2::KEY_A:
			case ps2::KEY_H: {
				state->snake.facing = Direction::Left;
			} break;
			case ps2::KEY_DOWN:
			case ps2::KEY_D:
			case ps2::KEY_J: {
				state->snake.facing = Direction::Down;
			} break;
			case ps2::KEY_UP:
			case ps2::KEY_W:
			case ps2::KEY_K: {
				state->snake.facing = Direction::Up;
			} break;
			case ps2::KEY_RIGHT:
			case ps2::KEY_S:
			case ps2::KEY_L: {
				state->snake.facing = Direction::Right;
			} break;
			default: break;
		}
	}
}

void draw(State &state) {
	term_clear();
	cursor_disable();

	term_goto(state.apple.x, state.apple.y);
	term_putchar('a');

	state.snake.draw();
}

void update(State &state) {
	state.snake.move();
}

}

void main() {
	State state{};
	state.apple = Pos::random_pos(state.prng);

	CallbackEventLoop event_loop{
		(CallbackEventLoop::cb_t)handle_keypress,
		&state,
	};

	draw(state);

	while (!state.should_quit) {
		auto _ = event_loop.get_frame(500); // 2 fps

		update(state);
		draw(state);
	}
}

}
