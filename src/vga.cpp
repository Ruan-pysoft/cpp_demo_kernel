#include "vga.hpp"

#include <stddef.h>
#include <stdint.h>

#include <string.h>
#include "ioport.hpp"

static size_t term_row;
static size_t term_col;
static vga_entry_color_t term_color;
static volatile vga_entry_t *const term_buffer = (vga_entry_t*)VGA_ADDR;
static bool move_cursor;

void term_init() {
	term_row = 0;
	term_col = 0;

	term_resetcolor();

	for (size_t y = 0; y < VGA_HEIGHT; ++y) {
		for (size_t x = 0; x < VGA_WIDTH; ++x) {
			const size_t index = y * VGA_WIDTH + x;
			term_buffer[index] = vga_entry(' ', term_color);
		}
	}

	move_cursor = true;
	cursor_enable(8, 15);
	cursor_goto(0, 0);
}
void term_setcolor(vga_entry_color_t color) {
	term_color = color;
}
vga_entry_color_t term_getcolor() {
	return term_color;
}
void term_resetcolor() {
	term_setcolor(vga_entry_color(vga_color::light_grey, vga_color::black));
}
void term_putentryat(vga_entry_t entry, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	term_buffer[index] = entry;
}
void term_putbyteat(uint8_t byte, vga_entry_color_t color, size_t x, size_t y) {
	term_putentryat(vga_entry(byte, color), x, y);
}
void term_scroll(size_t lines) {
	for (size_t y = lines; y < VGA_HEIGHT; ++y) {
		for (size_t x = 0; x < VGA_WIDTH; ++x) {
			const size_t index = y * VGA_WIDTH + x;
			const size_t prev_index = (y-lines) * VGA_WIDTH + x;

			term_buffer[prev_index] = term_buffer[index];
		}
	}
	for (size_t y = VGA_HEIGHT - lines; y < VGA_HEIGHT; ++y) {
		for (size_t x = 0; x < VGA_WIDTH; ++x) {
			const size_t index = y * VGA_WIDTH + x;

			term_buffer[index] = vga_entry(' ', term_color);
		}
	}
	term_row -= lines;
	if (move_cursor) cursor_goto(term_col, term_row);
}
void term_advance() {
	if (++term_col == VGA_WIDTH) {
		term_col = 0;
		if (++term_row == VGA_HEIGHT) term_scroll(2);
	}
	if (move_cursor) cursor_goto(term_col, term_row);
}
void term_putbyte(uint8_t byte) {
	term_putbyteat(byte, term_color, term_col, term_row);
	term_advance();
}
void term_putchar(char c) {
	if (c == '\n') {
		term_col = 0;
		if (++term_row == VGA_HEIGHT) term_scroll(2);
		if (move_cursor) cursor_goto(term_col, term_row);
	} else {
		term_putbyte(uint8_t(c));
	}
}
void term_backspace() {
	if (term_col == 0) {
		if (term_row == 0) return; // no back buffer, so can't go further back
		else {
			--term_row;
			term_col = VGA_WIDTH - 1;
		}
	} else --term_col;

	term_putbyteat(' ', term_color, term_col, term_row);
	if (move_cursor) cursor_goto(term_col, term_row);
}
void term_write_raw(const uint8_t *data, size_t size) {
	const bool did_move_cursor = move_cursor;
	move_cursor = false;

	for (size_t i = 0; i < size; ++i) {
		term_putbyte(data[i]);
	}

	move_cursor = did_move_cursor;
	if (move_cursor) cursor_goto(term_col, term_row);
}
void term_write(const char *data, size_t size) {
	const bool did_move_cursor = move_cursor;
	move_cursor = false;

	for (size_t i = 0; i < size; ++i) {
		term_putchar(data[i]);
	}

	move_cursor = did_move_cursor;
	if (move_cursor) cursor_goto(term_col, term_row);
}
void term_writestring(const char *str) {
	term_write(str, strlen(str));
}

void cursor_enable(uint8_t cursor_start, uint8_t cursor_end) {
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}
void cursor_disable() {
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}
void cursor_goto(size_t x, size_t y) {
	const uint16_t pos = y * VGA_WIDTH + x;

	outb(0x3D4, 0x0F);
	outb(0x3D5, uint8_t(pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, uint8_t(pos >> 8));
}
uint16_t cursor_getpos() {
	uint16_t pos = 0;

	outb(0x3D4, 0x0F);
	pos |= inb(0x3D5);
	outb(0x3D4, 0x0E);
	pos |= uint16_t(inb(0x3D5)) << 8;
	return pos;
}
size_t cursor_getx() {
	return cursor_getpos() % VGA_WIDTH;
}
size_t cursor_gety() {
	return cursor_getpos() / VGA_WIDTH;
}
uint16_t cursor_getxy() {
	const uint16_t pos = cursor_getpos();
	const uint8_t y = pos / VGA_WIDTH;
	const uint8_t x = pos - (y * VGA_WIDTH);

	return x | (uint16_t(y) << 8);
}
