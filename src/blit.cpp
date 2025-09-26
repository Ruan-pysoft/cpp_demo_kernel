#include "blit.hpp"

constexpr uint16_t blt_chr(char c) {
	return BLT_CHR(c);
}
void blt_put_chr(size_t ix, char c) {
	BLT_PUT_CHR(ix, c);
}
void blt_put_str(size_t ix, char *s, size_t l) {
	BLT_PUT_STR(ix, s, l);
}
void blt_put_hex(size_t ix, uint32_t x) {
	BLT_PUT_HEX(ix, x);
}

void blt_write_chr(char c) {
	BLT_WRITE_CHR(c);
}
void blt_write_str(char *s, size_t l) {
	BLT_WRITE_STR(s, l);
}
void blt_write_hex(uint32_t x) {
	BLT_WRITE_HEX(x);
}

constexpr size_t blt_idx(uint32_t x, uint32_t y) {
	return BLT_IDX(x, y);
}
uint32_t blt_x(void) {
	return BLT_X;
}
uint32_t blt_y(void) {
	return BLT_Y;
}

void blt_wrap() {
	BLT_WRAP();
}
void blt_newline() {
	BLT_NEWLINE();
}

namespace blit {
	size_t vga_idx = 0;
}
