#pragma once

#include <sdk/util.hpp>

#include "ps2.hpp"

struct Line {
	size_t pos;
	size_t len;
	size_t term_span;
};

class Pager {
	const char *text;
	sdk::util::List<Line> lines;
	size_t top_line = 0;
	size_t sub_line = 0;
public:
	bool should_quit = false;

	Pager(const char *text);

	void draw() const;
	void handle_key(ps2::Event key);
	static void handle_key_of(Pager &pager, ps2::Event key);

	void move_down_visual();
	void move_up_visual();
	void move_down_line();
	void move_up_line();
	void page_down();
	void page_up();
	void home();
	void end();
};
