#pragma once

#include <stddef.h>
#include <stdint.h>

#include "vga.hpp"

// for abort and interrupts, etc, when you don't want to jump into user code and deal with a cursor

#define BLT_CHR(c) (0xC000 | uint16_t(c))
#define BLT_PUT_CHR(ix, c) ((uint16_t*)VGA_ADDR)[ix] = BLT_CHR(c)
#define BLT_PUT_STR(ix, s, l) do { \
		for (size_t _blt_str_idx = 0; _blt_str_idx < l; ++_blt_str_idx) { \
			BLT_PUT_CHR(ix + _blt_str_idx, s[_blt_str_idx]); \
		} \
	} while (0);
#define BLT_PUT_HEX(ix, x) do { \
		uint32_t _blt_hex = x; \
		for (size_t i = 0; i < 8; ++i) { \
			const uint8_t _blt_digit = _blt_hex&0xF; \
			if (_blt_digit < 10) BLT_PUT_CHR(ix+8-i-1, '0' + _blt_digit); \
			else BLT_PUT_CHR(ix+8-i-1, 'a' - 10 + _blt_digit); \
			_blt_hex >>= 4; \
		} \
	} while (0);

extern "C" constexpr uint16_t blt_chr(char c);
extern "C" void blt_put_chr(size_t ix, char c);
extern "C" void blt_put_str(size_t ix, char *s, size_t l);
extern "C" void blt_put_hex(size_t ix, uint32_t x);

#define BLT_WRITE_CHR(c) do { BLT_PUT_CHR(blit::vga_idx, c); ++blit::vga_idx; } while (0)
#define BLT_WRITE_STR(s, l) do { BLT_PUT_STR(blit::vga_idx, s, l); blit::vga_idx += l; } while (0)
#define BLT_WRITE_HEX(x) do { BLT_PUT_HEX(blit::vga_idx, x); blit::vga_idx += 8; } while (0)

extern "C" void blt_write_chr(char c);
extern "C" void blt_write_str(char *s, size_t l);
extern "C" void blt_write_hex(uint32_t x);

#define BLT_IDX(x, y) (y * VGA_WIDTH + x)
#define BLT_X (blit::vga_idx % VGA_WIDTH)
#define BLT_Y (blit::vga_idx / VGA_WIDTH)

extern "C" constexpr size_t blt_idx(uint32_t x, uint32_t y);
extern "C" uint32_t blt_x(void);
extern "C" uint32_t blt_y(void);

#define BLT_WRAP() do { if (blit::vga_idx >= VGA_WIDTH*VGA_HEIGHT) blit::vga_idx -= VGA_WIDTH*VGA_HEIGHT; } while (0)
#define BLT_NEWLINE() do { blit::vga_idx += VGA_WIDTH; blit::vga_idx -= BLT_X; BLT_WRAP(); } while (0)

extern "C" void blt_wrap(void);
extern "C" void blt_newline(void);

namespace blit {
	extern size_t vga_idx;
}
