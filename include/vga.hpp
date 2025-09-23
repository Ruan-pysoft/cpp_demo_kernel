#pragma once

#include <stddef.h>
#include <stdint.h>

constexpr size_t VGA_WIDTH  = 80;
constexpr size_t VGA_HEIGHT = 25;
constexpr size_t VGA_ADDR   = 0xB8000;

/* Hardware text mode color constants. */
enum class vga_color : uint8_t {
	black = 0,
	blue = 1,
	green = 2,
	cyan = 3,
	red = 4,
	magenta = 5,
	brown = 6,
	light_grey = 7,
	dark_grey = 8,
	light_blue = 9,
	light_green = 10,
	light_cyan = 11,
	light_red = 12,
	light_magenta = 13,
	light_brown = 14,
	white = 15,
};

using vga_entry_color_t = uint8_t;
using vga_entry_t = uint16_t;

constexpr static inline vga_entry_color_t vga_entry_color(vga_color fg, vga_color bg) {
	return vga_entry_color_t(fg) | (vga_entry_color_t(bg) << 4);
}
constexpr static inline vga_color vga_bgcolor(vga_entry_color_t color) {
	return vga_color(color >> 4);
}
constexpr static inline vga_color vga_fgcolor(vga_entry_color_t color) {
	return vga_color(color & 0xF);
}
constexpr static inline vga_entry_t vga_entry(uint8_t b, vga_entry_color_t color) {
	return vga_entry_t(b) | (vga_entry_t(color) << 8);
}

void term_init();
void term_setcolor(vga_entry_color_t color);
vga_entry_color_t term_getcolor();
void term_resetcolor();
void term_putentryat(vga_entry_t entry, size_t x, size_t y); /* WARN: no bounds checking */
void term_putbyteat(uint8_t byte, vga_entry_color_t color, size_t x, size_t y); /* WARN: no bounds checking */
void term_scroll(size_t lines);
void term_advance();
void term_putbyte(uint8_t byte);
void term_putchar(char c);
void term_write_raw(const uint8_t *data, size_t size);
void term_write(const char *data, size_t size);
void term_writestring(const char *str);

void cursor_enable(uint8_t cursor_start, uint8_t cursor_end);
void cursor_disable(uint8_t cursor_start, uint8_t cursor_end);
void cursor_goto(size_t x, size_t y);
uint16_t cursor_getpos();
size_t cursor_getx();
size_t cursor_gety();
uint16_t cursor_getxy(); /* x = res&0xFF, y = res>>8 */
