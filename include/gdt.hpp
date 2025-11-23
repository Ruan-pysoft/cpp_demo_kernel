#pragma once

#include <stdint.h>

namespace gdt {

namespace _ {

static constexpr uint16_t create_segment_selector(uint16_t index, bool use_local, uint8_t privilege_level) {
	return (index<<3) | uint16_t(use_local<<2) | (privilege_level&0b11);
}

};

static constexpr uint16_t kcode_segment = _::create_segment_selector(1, false, 0);
static constexpr uint16_t kdata_segment = _::create_segment_selector(2, false, 0);

void init();
void load();

};
