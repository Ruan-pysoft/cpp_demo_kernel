#pragma once

#include <stddef.h>
#include <stdint.h>

namespace vga {

constexpr size_t WIDTH  = 80;
constexpr size_t HEIGHT = 25;
constexpr size_t ADDR   = 0xB8000;

/* Hardware text mode color constants. */
enum class Color : uint8_t {
	Black = 0,
	Blue = 1,
	Green = 2,
	Cyan = 3,
	Red = 4,
	Magenta = 5,
	Brown = 6,
	LightGrey = 7,
	DarkGrey = 8,
	LightBlue = 9,
	LightGreen = 10,
	LightCyan = 11,
	LightRed = 12,
	LightMagenta = 13,
	LightBrown = 14,
	White = 15,
};

using entry_color_t = uint8_t;
using entry_t = uint16_t;

constexpr static inline entry_color_t entry_color(Color fg, Color bg) {
	return entry_color_t(fg) | (entry_color_t(bg) << 4);
}
constexpr static inline Color bgcolor(entry_color_t color) {
	return Color(color >> 4);
}
constexpr static inline Color fgcolor(entry_color_t color) {
	return Color(color & 0xF);
}
constexpr static inline entry_t entry(uint8_t b, entry_color_t color) {
	return entry_t(b) | (entry_t(color) << 8);
}

}

namespace term {

void init();
void clear();
void go_to(size_t x, size_t y);
void setcolor(vga::entry_color_t color);
vga::entry_color_t getcolor();
void resetcolor();
void putentryat(vga::entry_t entry, size_t x, size_t y); /* WARN: no bounds checking */
void putbyteat(uint8_t byte, vga::entry_color_t color, size_t x, size_t y); /* WARN: no bounds checking */
void scroll(size_t lines);
void advance();
void putbyte(uint8_t byte);
void putchar(char c);
void backspace();
void write_raw(const uint8_t *data, size_t size);
void write(const char *data, size_t size);
void writestring(const char *str);

namespace cursor {

void enable(uint8_t cursor_start, uint8_t cursor_end);
void disable();
void go_to(size_t x, size_t y);
uint16_t getpos();
size_t getx();
size_t gety();
uint16_t getxy(); /* x = res&0xFF, y = res>>8 */

}

}
