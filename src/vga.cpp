#include "vga.hpp"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <string.h>
#include "ioport.hpp"

namespace term {

namespace {

size_t row;
size_t col;
vga::entry_color_t color;
volatile vga::entry_t *const vga_buffer = (vga::entry_t*)vga::ADDR;
bool move_cursor;
bool autoscroll;
volatile vga::entry_t *buffer;

}

size_t Backbuffer::instance_count = 0;
Backbuffer::Backbuffer() : was_moving_cursor(move_cursor) {
	if (instance_count == 0) {
		::term::buffer = this->buffer;
		memcpy(this->buffer, (void*)vga_buffer, sizeof(buffer));
	}
	++instance_count;
	move_cursor = false;
}
Backbuffer::~Backbuffer() {
	--instance_count;
	if (instance_count == 0) {
		::term::buffer = vga_buffer;
		memcpy((void*)vga_buffer, buffer, sizeof(buffer));
	}
	if (was_moving_cursor) {
		move_cursor = true;
		cursor::go_to(col, row);
	}
}

void init() {
	row = 0;
	col = 0;

	buffer = vga_buffer;

	resetcolor();

	clear();

	move_cursor = true;
	cursor::enable(8, 15);
	cursor::go_to(0, 0);

	enable_autoscroll();
}
void clear() {
	for (size_t y = 0; y < vga::HEIGHT; ++y) {
		for (size_t x = 0; x < vga::WIDTH; ++x) {
			const size_t index = y * vga::WIDTH + x;
			buffer[index] = vga::entry(' ', color);
		}
	}
}
void go_to(size_t x, size_t y) {
	col = x;
	row = y;
	if (col > vga::WIDTH) col = vga::WIDTH-1;
	if (row > vga::HEIGHT) row = vga::HEIGHT-1;
	if (move_cursor) cursor::go_to(col, row);
}
void setcolor(vga::entry_color_t color) {
	::term::color = color;
}
vga::entry_color_t getcolor() {
	return color;
}
void resetcolor() {
	setcolor(vga::entry_color(vga::Color::LightGrey, vga::Color::Black));
}
void putentryat(vga::entry_t entry, size_t x, size_t y) {
	const size_t index = y * vga::WIDTH + x;
	buffer[index] = entry;
}
void putbyteat(uint8_t byte, vga::entry_color_t color, size_t x, size_t y) {
	putentryat(vga::entry(byte, color), x, y);
}
void enable_autoscroll() {
	autoscroll = true;
}
void disable_autoscroll() {
	autoscroll = false;
}
void scroll(size_t lines) {
	for (size_t y = lines; y < vga::HEIGHT; ++y) {
		for (size_t x = 0; x < vga::WIDTH; ++x) {
			const size_t index = y * vga::WIDTH + x;
			const size_t prev_index = (y-lines) * vga::WIDTH + x;

			buffer[prev_index] = buffer[index];
		}
	}
	for (size_t y = vga::HEIGHT - lines; y < vga::HEIGHT; ++y) {
		for (size_t x = 0; x < vga::WIDTH; ++x) {
			const size_t index = y * vga::WIDTH + x;

			buffer[index] = vga::entry(' ', color);
		}
	}
	row -= lines;
	if (move_cursor) cursor::go_to(col, row);
}
void advance() {
	if (++col == vga::WIDTH) {
		col = 0;
		if (++row == vga::HEIGHT) {
			if (autoscroll) scroll(2);
			else row = 0;
		}
	}
	if (move_cursor) cursor::go_to(col, row);
}
void putbyte(uint8_t byte) {
	putbyteat(byte, color, col, row);
	advance();
}
void putchar(char c) {
	if (c == '\n') {
		col = 0;
		if (++row == vga::HEIGHT) {
			if (autoscroll) scroll(2);
			else row = 0;
		}
		if (move_cursor) cursor::go_to(col, row);
	} else {
		putbyte(uint8_t(c));
	}
}
void backspace() {
	if (col == 0) {
		if (row == 0) return; // no back buffer, so can't go further back
		else {
			--row;
			col = vga::WIDTH - 1;
		}
	} else --col;

	putbyteat(' ', color, col, row);
	if (move_cursor) cursor::go_to(col, row);
}
void write_raw(const uint8_t *data, size_t size) {
	const bool did_move_cursor = move_cursor;
	move_cursor = false;

	for (size_t i = 0; i < size; ++i) {
		putbyte(data[i]);
	}

	move_cursor = did_move_cursor;
	if (move_cursor) cursor::go_to(col, row);
}
void write(const char *data, size_t size) {
	const bool did_move_cursor = move_cursor;
	move_cursor = false;

	for (size_t i = 0; i < size; ++i) {
		putchar(data[i]);
	}

	move_cursor = did_move_cursor;
	if (move_cursor) cursor::go_to(col, row);
}
void writestring(const char *str) {
	write(str, strlen(str));
}

namespace cursor {

namespace {

uint8_t cursor_start = 0;
uint8_t cursor_end = 0;
bool enabled = false;

}

uint8_t start() {
	return cursor_start;
}
uint8_t end() {
	return cursor_end;
}
bool is_enabled() {
	return enabled;
}
void enable() {
	enable(cursor_start, cursor_end);
}
void enable(uint8_t start, uint8_t end) {
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | end);

	enabled = true;
	cursor_start = start;
	cursor_end = end;
}
void disable() {
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);

	enabled = false;
}
void go_to(size_t x, size_t y) {
	const uint16_t pos = y * vga::WIDTH + x;

	outb(0x3D4, 0x0F);
	outb(0x3D5, uint8_t(pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, uint8_t(pos >> 8));
}
uint16_t getpos() {
	uint16_t pos = 0;

	outb(0x3D4, 0x0F);
	pos |= inb(0x3D5);
	outb(0x3D4, 0x0E);
	pos |= uint16_t(inb(0x3D5)) << 8;
	return pos;
}
size_t getx() {
	return cursor::getpos() % vga::WIDTH;
}
size_t gety() {
	return cursor::getpos() / vga::WIDTH;
}
uint16_t getxy() {
	const uint16_t pos = cursor::getpos();
	const uint8_t y = pos / vga::WIDTH;
	const uint8_t x = pos - (y * vga::WIDTH);

	return x | (uint16_t(y) << 8);
}

}

}
