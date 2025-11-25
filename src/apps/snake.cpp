#include "apps/snake.hpp"

#include <assert.h>
#include <sdk/eventloop.hpp>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pit.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace snake {

namespace {

constexpr size_t STAGE_WIDTH = vga::WIDTH/2;
constexpr size_t STAGE_HEIGHT = vga::HEIGHT;

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

	inline void go_to() const {
		term::go_to(x*2, y);
	}

	inline void move(Direction dir) {
		*this = moved(dir);
	}
	inline Pos moved(Direction dir) const {
		Pos res = *this;
		switch (dir) {
			case Direction::Left: {
				res.x = res.x == 0
					? STAGE_WIDTH - 1 : res.x - 1;
			} break;
			case Direction::Down: {
				res.y = res.y == STAGE_HEIGHT - 1
					? 0 : res.y + 1;
			} break;
			case Direction::Up: {
				res.y = res.y == 0
					? STAGE_HEIGHT - 1 : res.y - 1;
			} break;
			case Direction::Right: {
				res.x = res.x == STAGE_WIDTH - 1
					? 0 : res.x + 1;
			} break;
		}
		return res;
	}

	static Pos random_pos(Prng &prng) {
		const uint32_t rand = prng.next();
		// technically biased towards lower numbers, but who cares?
		return {
			.x = (uint8_t)((rand&0xFF) % STAGE_WIDTH),
			.y = (uint8_t)(((rand>>8)&0xFF) % STAGE_HEIGHT),
		};
	}
};

struct State;
constexpr size_t MAX_SNAKE_SIZE = STAGE_WIDTH*STAGE_HEIGHT;
class Snake {
	Pos segments[MAX_SNAKE_SIZE];
	size_t num_segments;

	Direction prev_facing;
	bool has_prev_facing = false;
	Direction facing;
public:
	Snake(Pos start = {STAGE_WIDTH/2, STAGE_HEIGHT/2}, size_t initial_segments = 5, Direction initial_dir = Direction::Left)
	: num_segments(initial_segments), facing(initial_dir) {
		assert(num_segments != 0);
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
		term::setcolor(vga::entry_color(
			vga::Color::Green,
			vga::Color::Black
		));

		size_t i = num_segments;
		while (i --> 1) {
			segments[i].go_to();
			term::putchar('[');
			term::putchar(']');
		}
		segments[0].go_to();
		term::putchar('(');
		term::putchar(')');

		term::resetcolor();
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

void handle_keypress(State &state, ps2::Event event) {
	if (event.type == ps2::EventType::Press) {
		switch (event.key) {
			case ps2::KEY_Q:
			case ps2::KEY_ESCAPE: {
				state.should_quit = true;
			} break;
			case ps2::KEY_R:
			case ps2::KEY_ENTER: {
				// only allow the player to restart from the game over screen;
				// if they want to restart in play, they just have to run into themselves
				if (state.lost) state.restart = true;
			} break;

			case ps2::KEY_LEFT:
			case ps2::KEY_A:
			case ps2::KEY_H: {
				state.snake.look(Direction::Left);
			} break;
			case ps2::KEY_DOWN:
			case ps2::KEY_D:
			case ps2::KEY_J: {
				state.snake.look(Direction::Down);
			} break;
			case ps2::KEY_UP:
			case ps2::KEY_W:
			case ps2::KEY_K: {
				state.snake.look(Direction::Up);
			} break;
			case ps2::KEY_RIGHT:
			case ps2::KEY_S:
			case ps2::KEY_L: {
				state.snake.look(Direction::Right);
			} break;
			default: break;
		}
	}
}

void draw(State &state) {
	using namespace term;

	clear();
	cursor::disable();

	go_to(2, 1);
	printf("Score: %d", state.score);

	state.apple.go_to();
	setcolor(vga::entry_color(
		vga::Color::Red,
		vga::Color::Black
	));
	putchar('\xa2');
	putchar('\x95');
	resetcolor();

	state.snake.draw();

	if (state.lost) {
		const char *lose_text =  " ** GAME OVER ** ";
		const char *frame_text = "                 ";
		const size_t lose_text_len = strlen(lose_text);
		const size_t lose_text_offset = (vga::WIDTH - lose_text_len)/2;

		if (state.blink_state) {
			setcolor(vga::entry_color(
				vga::Color::White,
				vga::Color::Red
			));
		} else {
			setcolor(vga::entry_color(
				vga::Color::Red,
				vga::Color::White
			));
		}

		go_to(lose_text_offset, vga::HEIGHT/2 - 1);
		writestring(frame_text);
		go_to(lose_text_offset, vga::HEIGHT/2);
		writestring(lose_text);
		go_to(lose_text_offset, vga::HEIGHT/2 + 1);
		writestring(frame_text);

		resetcolor();
	}
}

void update(State &state) {
	if (!state.lost) state.snake.update(state);
	state.blink_state = !state.blink_state;
}

void Snake::update(State &state) {
	has_prev_facing = true;
	prev_facing = facing;

	Pos next_head_pos = segments[0].moved(facing);

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
	term::disable_autoscroll();

	bool should_quit = false;

	while (!should_quit) {
		State state{};
		state.apple = Pos::random_pos(state.prng);

		sdk::CallbackEventLoop<State&> event_loop {
			handle_keypress,
			state,
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

	term::enable_autoscroll();
}

}
