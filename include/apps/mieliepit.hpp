#pragma once

#include <sdk/util.hpp>

namespace mieliepit {

struct State;
class Interface {
	State *state;

public:
	Interface();
	~Interface();

	void run(const char *str);
	void run(const sdk::util::String &str);
};

void main();
void main(Interface &interface);

}
