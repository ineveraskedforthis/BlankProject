#include "text_render.hpp"

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
) {
	auto& current_font = font_collection.get_font(font_id);
	if(std::holds_alternative<text::embedded_icon>(t.source)) {
		ogl::render_text_icon(
			state,
			font_collection,
			std::get<text::embedded_icon>(t.source),
			x,
			baseline_y,
			float(font_size),
			current_font,
			cmod,
			ui_scale
		);
	} else {
                render_new_text(
                        state,
                        font_collection,
                        current_font,
                        t.unicodechars,
                        cmod,
                        x,
                        baseline_y,
                        float(font_size),
                        text_color,
                        ui_scale
                );
	}
}
}