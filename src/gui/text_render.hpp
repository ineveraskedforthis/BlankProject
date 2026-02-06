#include "opengl_wrapper.hpp"
#include "text.hpp"

namespace ui {
void render_text_chunk(
	text::font_manager& font_collection,
        ogl::data& state,
	text::text_chunk t,
	float x,
	float baseline_y,
	uint16_t font_size,
	uint16_t font_id,
	ogl::color3f text_color,
	ogl::color_modification cmod,
        float ui_scale
);
}