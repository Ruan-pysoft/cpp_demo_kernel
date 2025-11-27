#include <sdk/terminal.hpp>

#include "vga.hpp"

namespace sdk {

ColorSwitch::ColorSwitch() : original_color(term::getcolor()) { }
ColorSwitch::ColorSwitch(vga::Color fg) : ColorSwitch() {
	term::setcolor(vga::entry_color(
		fg,
		vga::bgcolor(original_color)
	));
}
ColorSwitch::ColorSwitch(vga::Color fg, vga::Color bg) : ColorSwitch() {
	term::setcolor(vga::entry_color(fg, bg));
}
ColorSwitch::~ColorSwitch() {
	term::setcolor(original_color);
}

void ColorSwitch::set_fg(vga::Color color) {
	term::setcolor(vga::entry_color(
		color,
		vga::bgcolor(original_color)
	));
}
void ColorSwitch::set_bg(vga::Color color) {
	term::setcolor(vga::entry_color(
		vga::fgcolor(original_color),
		color
	));
}
void ColorSwitch::set(vga::Color fg, vga::Color bg) {
	set(vga::entry_color(fg, bg));
}
void ColorSwitch::set(vga::entry_color_t colors) {
	term::setcolor(colors);
}
void ColorSwitch::reset() {
	term::setcolor(original_color);
}

void ColorSwitch::with_fg(vga::Color color, void(*fn)()) {
	with(color, vga::bgcolor(original_color), fn);
}
void ColorSwitch::with_bg(vga::Color color, void(*fn)()) {
	with(vga::fgcolor(original_color), color, fn);
}
void ColorSwitch::with(vga::Color fg, vga::Color bg, void(*fn)()) {
	with(vga::entry_color(fg, bg), fn);
}
void ColorSwitch::with(vga::entry_color_t colors, void(*fn)()) {
	set(colors);
	fn();
	reset();
}

}
