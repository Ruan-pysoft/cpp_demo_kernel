#pragma once

#include <stddef.h>

#include "vga.hpp"

namespace sdk {

class ColorSwitch {
	vga::entry_color_t original_color;
public:
	ColorSwitch();
	ColorSwitch(vga::Color fg);
	ColorSwitch(vga::Color fg, vga::Color bg);
	~ColorSwitch();

	void set_fg(vga::Color color);
	void set_bg(vga::Color color);
	void set(vga::Color fg, vga::Color bg);
	void set(vga::entry_color_t colors);
	void reset();

	void with_fg(vga::Color color, void(*fn)());
	void with_bg(vga::Color color, void(*fn)());
	void with(vga::Color fg, vga::Color bg, void(*fn)());
	void with(vga::entry_color_t colors, void(*fn)());

	template<typename T>
	void with_fg(vga::Color color, void(*fn)(T), T arg) {
		with(color, vga::bgcolor(original_color), fn, arg);
	}
	template<typename T>
	void with_bg(vga::Color color, void(*fn)(T), T arg) {
		with(vga::fgcolor(original_color), color, fn, arg);
	}
	template<typename T>
	void with(vga::Color fg, vga::Color bg, void(*fn)(T), T arg) {
		with(vga::entry_color(fg, bg), fn, arg);
	}
	template<typename T>
	void with(vga::entry_color_t colors, void(*fn)(T), T arg) {
		set(colors);
		fn(arg);
		reset();
	}
};

namespace colors {

inline void with_fg(vga::Color color, void(*fn)()) {
	ColorSwitch().with_fg(color, fn);
}
inline void with_bg(vga::Color color, void(*fn)()) {
	ColorSwitch().with_bg(color, fn);
}
inline void with(vga::Color fg, vga::Color bg, void(*fn)()) {
	ColorSwitch().with(fg, bg, fn);
}
inline void with(vga::entry_color_t colors, void(*fn)()) {
	ColorSwitch().with(colors, fn);
}

template<typename T>
inline void with_fg(vga::Color color, void(*fn)(T), T arg) {
	ColorSwitch().with_fg(color, fn, arg);
}
template<typename T>
inline void with_bg(vga::Color color, void(*fn)(T), T arg) {
	ColorSwitch().with_bg(color, fn, arg);
}
template<typename T>
inline void with(vga::Color fg, vga::Color bg, void(*fn)(T), T arg) {
	ColorSwitch().with(fg, bg, fn, arg);
}
template<typename T>
inline void with(vga::entry_color_t colors, void(*fn)(T), T arg) {
	ColorSwitch().with(colors, fn, arg);
}

}

}
