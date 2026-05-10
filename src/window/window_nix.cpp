#include "window.hpp"
//#include "map.hpp"
#include "user_interactions.hpp"

#include <GLFW/glfw3.h>
#include <unordered_map>
#include <cmath>

namespace window {

int32_t cursor_blink_ms() {
	return 1000;
}
int32_t double_click_ms() {
	return 500;
}

void change_cursor(sys::state& state, cursor_type type) {
	//TODO: Implement on linux
}

void emit_error_message(std::string const& content, bool fatal) {
	std::fprintf(stderr, "%s", content.c_str());
	if(fatal) {
		std::exit(EXIT_FAILURE);
	}
}

} // namespace window
