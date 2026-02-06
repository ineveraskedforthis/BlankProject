#include <algorithm>
#include <cmath>
#include <stddef.h>
#include <stdint.h>
#include <variant>
#include "dcon_generated_ids.hpp"
#include "gui_element_base.hpp"
#include "gui_element_types.hpp"
#include "fonts.hpp"
#include "gui_graphics.hpp"
#include "opengl_wrapper.hpp"
#include "text.hpp"
#include "sound.hpp"
#include "alice_ui.hpp"
#include "text_utility.hpp"

namespace ui {

inline message_result greater_result(message_result a, message_result b) {
	if(a == message_result::consumed || b == message_result::consumed)
		return message_result::consumed;
	if(a == message_result::seen || b == message_result::seen)
		return message_result::seen;
	return message_result::unseen;
}

mouse_probe container_base::impl_probe_mouse(sys::state& state, int32_t x, int32_t y, mouse_probe_type type) noexcept {
	for(auto& c : children) {
		if(c->is_visible()) {
			auto relative_location = child_relative_location(state, *this, *c);
			auto res = c->impl_probe_mouse(state, x - relative_location.x, y - relative_location.y, type);
			if(res.under_mouse)
				return res;
		}
	}
	return element_base::impl_probe_mouse(state, x, y, type);
}

drag_and_drop_query_result container_base::impl_drag_and_drop_query(sys::state& state, int32_t x, int32_t y, ui::drag_and_drop_data data_type) noexcept {
	for(auto& c : children) {
		if(c->is_visible()) {
			auto relative_location = child_relative_location(state, *this, *c);
			auto res = c->impl_drag_and_drop_query(state, x - relative_location.x, y - relative_location.y, data_type);
			if(res.under_mouse)
				return res;
		}
	}
	return element_base::impl_drag_and_drop_query(state, x, y, data_type);
}

mouse_probe non_owning_container_base::impl_probe_mouse(sys::state& state, int32_t x, int32_t y, mouse_probe_type type) noexcept {
	for(auto& c : children) {
		if(c->is_visible()) {
			auto relative_location = child_relative_location(state, *this, *c);
			auto res = c->impl_probe_mouse(state, x - relative_location.x, y - relative_location.y, type);
			if(res.under_mouse)
				return res;
		}
	}
	return element_base::impl_probe_mouse(state, x, y, type);
}

drag_and_drop_query_result non_owning_container_base::impl_drag_and_drop_query(sys::state& state, int32_t x, int32_t y, ui::drag_and_drop_data data_type) noexcept {
	for(auto& c : children) {
		if(c->is_visible()) {
			auto relative_location = child_relative_location(state, *this, *c);
			auto res = c->impl_drag_and_drop_query(state, x - relative_location.x, y - relative_location.y, data_type);
			if(res.under_mouse)
				return res;
		}
	}
	return element_base::impl_drag_and_drop_query(state, x, y, data_type);
}

message_result container_base::impl_on_key_down(sys::state& state, sys::virtual_key key, sys::key_modifiers mods) noexcept {
	message_result res = message_result::unseen;
	for(auto& c : children) {
		if(c->is_visible()) {
			res = greater_result(res, c->impl_on_key_down(state, key, mods));
			if(res == message_result::consumed)
				return message_result::consumed;
		}
	}
	return greater_result(res, element_base::impl_on_key_down(state, key, mods));
}
message_result non_owning_container_base::impl_on_key_down(sys::state& state, sys::virtual_key key, sys::key_modifiers mods) noexcept {
	message_result res = message_result::unseen;
	for(auto& c : children) {
		if(c->is_visible()) {
			res = greater_result(res, c->impl_on_key_down(state, key, mods));
			if(res == message_result::consumed)
				return message_result::consumed;
		}
	}
	return greater_result(res, element_base::impl_on_key_down(state, key, mods));
}

void container_base::impl_on_update(sys::state& state) noexcept {
	on_update(state);
	if(is_visible()) {
		for(auto& c : children) {
			if(c->is_visible() || (c->flags & element_base::wants_update_when_hidden_mask) != 0) {
				c->impl_on_update(state);
			}
		}
	}
}
void non_owning_container_base::impl_on_update(sys::state& state) noexcept {
	on_update(state);
	if(is_visible()) {
		for(size_t i = children.size(); i-- > 0;) {
			if(children[i]->is_visible() || (children[i]->flags & element_base::wants_update_when_hidden_mask) != 0) {
				children[i]->impl_on_update(state);
			}
		}
	}
}
void container_base::impl_on_reset_text(sys::state& state) noexcept {
	for(auto& c : children) {
		c->impl_on_reset_text(state);
	}
	on_reset_text(state);
}
void non_owning_container_base::impl_on_reset_text(sys::state& state) noexcept {
	for(auto& c : children) {
		c->impl_on_reset_text(state);
	}
	on_reset_text(state);
}
void container_base::impl_render(sys::state& state, int32_t x, int32_t y) noexcept {
	element_base::impl_render(state, x, y);

	for(size_t i = children.size(); i-- > 0;) {
		if(children[i]->is_visible()) {
			auto relative_location = child_relative_location(state, *this, *(children[i]));
			children[i]->impl_render(state, x + relative_location.x, y + relative_location.y);
		}
	}
}
void non_owning_container_base::impl_render(sys::state& state, int32_t x, int32_t y) noexcept {
	element_base::impl_render(state, x, y);

	for(size_t i = children.size(); i-- > 0;) {
		if(children[i]->is_visible()) {
			auto relative_location = child_relative_location(state, *this, *(children[i]));
			children[i]->impl_render(state, x + relative_location.x, y + relative_location.y);
		}
	}
}

std::unique_ptr<element_base> container_base::remove_child(element_base* child) noexcept {
	if(auto it = std::find_if(children.begin(), children.end(), [child](std::unique_ptr<element_base>& p) { return p.get() == child; }); it != children.end()) {
		if(it + 1 != children.end())
			std::rotate(it, it + 1, children.end());
		auto temp = std::move(children.back());
		children.pop_back();
		temp->parent = nullptr;
		return temp;
	}
	return std::unique_ptr<element_base>{};
}
void container_base::move_child_to_front(element_base* child) noexcept {
	if(auto it = std::find_if(children.begin(), children.end(), [child](std::unique_ptr<element_base>& p) { return p.get() == child; }); it != children.end()) {
		if(it != children.begin())
			std::rotate(children.begin(), it, it + 1);
	}
}
void non_owning_container_base::move_child_to_front(element_base* child) noexcept {
	if(auto it = std::find_if(children.begin(), children.end(), [child](element_base* p) { return p == child; }); it != children.end()) {
		if(it != children.begin())
			std::rotate(children.begin(), it, it + 1);
	}
}
void container_base::move_child_to_back(element_base* child) noexcept {
	if(auto it = std::find_if(children.begin(), children.end(), [child](std::unique_ptr<element_base>& p) { return p.get() == child; }); it != children.end()) {
		if(it + 1 != children.end())
			std::rotate(it, it + 1, children.end());
	}
}
void non_owning_container_base::move_child_to_back(element_base* child) noexcept {
	if(auto it = std::find_if(children.begin(), children.end(), [child](element_base* p) { return p == child; }); it != children.end()) {
		if(it + 1 != children.end())
			std::rotate(it, it + 1, children.end());
	}
}
void container_base::add_child_to_front(std::unique_ptr<element_base> child) noexcept {
	child->parent = this;
	children.emplace_back(std::move(child));
	if(children.size() > 1) {
		std::rotate(children.begin(), children.end() - 1, children.end());
	}
}
void container_base::add_child_to_back(std::unique_ptr<element_base> child) noexcept {
	child->parent = this;
	children.emplace_back(std::move(child));
}
element_base* container_base::get_child_by_index(sys::state const& state, int32_t index) noexcept {
	if(0 <= index && index < int32_t(children.size()))
		return children[index].get();
	return nullptr;
}
element_base* non_owning_container_base::get_child_by_index(sys::state const& state, int32_t index) noexcept {
	if(0 <= index && index < int32_t(children.size()))
		return children[index];
	return nullptr;
}


// From ui_f_shader.glsl
uint32_t internal_get_interactable_disabled_color(float r, float g, float b) {
	float amount = (r + g + b) / 4.f;
	return sys::pack_color(std::min(1.f, amount + 0.1f), std::min(1.f, amount + 0.1f), std::min(1.f, amount + 0.1f));
}
uint32_t internal_get_interactable_color(float r, float g, float b) {
	return sys::pack_color(std::min(1.f, r + 0.1f), std::min(1.f, g + 0.1f), std::min(1.f, b + 0.1f));
}
uint32_t internal_get_disabled_color(float r, float g, float b) {
	float amount = (r + g + b) / 4.f;
	return sys::pack_color(amount, amount, amount);
}

void edit_box_element_base::internal_move_cursor_to_point(sys::state& state, int32_t x, int32_t y, bool extend_selection) {
	int32_t hmargin = 0;
	int32_t vmargin = 0;
	uint16_t fonthandle = 0;
	float lineheight = 0;

	if(template_id != -1) {
		alice_ui::grid_size_window* par = static_cast<alice_ui::grid_size_window*>(parent);
		hmargin = int32_t(state.ui_templates.button_t[template_id].primary.h_text_margins * par->grid_size);

		fonthandle = text::make_font_id(state, state.ui_templates.button_t[template_id].primary.font_choice == 1, state.ui_templates.button_t[template_id].primary.font_scale * par->grid_size * 2);
		lineheight = state.font_collection.line_height(state, fonthandle);
		switch(state.ui_templates.button_t[template_id].primary.v_text_alignment) {
		case template_project::aui_text_alignment::center: vmargin = int32_t((base_data.size.y - lineheight) / 2); break;
		case template_project::aui_text_alignment::left: vmargin = int32_t(state.ui_templates.button_t[template_id].primary.v_text_margins * par->grid_size);  break;
		case template_project::aui_text_alignment::right: vmargin = int32_t(base_data.size.y - lineheight - state.ui_templates.button_t[template_id].primary.v_text_margins * par->grid_size); break;
		}
	}

	auto xpos = x - hmargin;
	auto ypos = y - vmargin;


	auto line = int32_t(ypos / lineheight);
	if(!multiline) {
		line = 0;
	} else {
		line = std::clamp(line, 0, std::max(0, int32_t(glyph_details.total_lines - 1)));
	}

	auto best_cursor_pos = best_cursor_fit_on_line(line, xpos);
	if(best_cursor_pos != -1) {
		cursor_position = best_cursor_pos;
		if(!extend_selection) {
			anchor_position = cursor_position;
		}
	}
}
void edit_box_element_base::set_temporary_selection(sys::state& state, int32_t start, int32_t end) noexcept {
	temp_selection_start = start;
	temp_selection_end = end;

	/*
	// TODO: accessibility
	if(acc_obj) {
		win.accessibility_interface.on_composition_change(acc_obj, std::wstring_view(text.data(), size_t(end - start)));
	}
	*/
}
std::pair<int32_t, int32_t> edit_box_element_base::temporary_text_range() noexcept {
	return std::pair<int32_t, int32_t>{temp_selection_start, temp_selection_end};
}
void edit_box_element_base::register_composition_result(sys::state& state) noexcept {
	/*
	// TODO accessibility
	if(acc_obj && win.is_visible(l_id)) {
			win.accessibility_interface.on_composition_result(acc_obj, std::wstring_view(text.data() + temp_text_position, size_t(temp_text_length)));
		}
	*/
	temp_selection_start = 0;
	temp_selection_end = 0;
}
std::pair<int32_t, int32_t> edit_box_element_base::text_selection() noexcept {
	if(glyph_details.grapheme_placement.empty())
		return std::pair<int32_t, int32_t>{0, 0};
	return std::pair<int32_t, int32_t>{
		cursor_position < int32_t(glyph_details.grapheme_placement.size()) ? glyph_details.grapheme_placement[std::max(cursor_position, 0)].source_offset : int32_t(cached_text.size()),
		anchor_position < int32_t(glyph_details.grapheme_placement.size()) ? glyph_details.grapheme_placement[std::max(anchor_position, 0)].source_offset : int32_t(cached_text.size())
	};
}
void edit_box_element_base::set_text_selection(sys::state& state, int32_t cursor, int32_t anchor) noexcept {
	cursor_position = int32_t(glyph_details.grapheme_placement.size());
	for(size_t j = glyph_details.grapheme_placement.size(); j-- > 0; ) {
		if(int32_t(glyph_details.grapheme_placement[j].source_offset) < cursor) {
			cursor_position = int32_t(j + 1);
			break;
		} else if(int32_t(glyph_details.grapheme_placement[j].source_offset) == cursor) {
			cursor_position = int32_t(j);
			break;
		}
	}
	anchor_position = int32_t(glyph_details.grapheme_placement.size());
	for(size_t j = glyph_details.grapheme_placement.size(); j-- > 0; ) {
		if(int32_t(glyph_details.grapheme_placement[j].source_offset) < anchor) {
			anchor_position = int32_t(j+1);
			break;
		} else if(int32_t(glyph_details.grapheme_placement[j].source_offset) == anchor) {
			anchor_position = int32_t(j);
			break;
		}
	}
	internal_on_selection_changed(state);
}
sys::text_mouse_test_result edit_box_element_base::detailed_text_mouse_test(sys::state& state, int32_t x, int32_t y) noexcept {
	int32_t hmargin = 0;
	int32_t vmargin = 0;
	uint16_t fonthandle = 0;
	float lineheight = 0;

	if(template_id != -1) {
		alice_ui::grid_size_window* par = static_cast<alice_ui::grid_size_window*>(parent);
		hmargin = int32_t(state.ui_templates.button_t[template_id].primary.h_text_margins * par->grid_size);

		fonthandle = text::make_font_id(state, state.ui_templates.button_t[template_id].primary.font_choice == 1, state.ui_templates.button_t[template_id].primary.font_scale * par->grid_size * 2);
		lineheight = state.font_collection.line_height(state, fonthandle);
		switch(state.ui_templates.button_t[template_id].primary.v_text_alignment) {
		case template_project::aui_text_alignment::center: vmargin = int32_t((base_data.size.y - lineheight) / 2); break;
		case template_project::aui_text_alignment::left: vmargin = int32_t(state.ui_templates.button_t[template_id].primary.v_text_margins * par->grid_size);  break;
		case template_project::aui_text_alignment::right: vmargin = int32_t(base_data.size.y - lineheight - state.ui_templates.button_t[template_id].primary.v_text_margins * par->grid_size); break;
		}
	}

	auto xpos = x - hmargin;
	auto ypos = y - vmargin;

	auto line = int32_t(ypos / lineheight);
	if(!multiline) {
		line = 0;
	} else {
		line = std::clamp(line, 0, std::max(0, int32_t(glyph_details.total_lines - 1)));
	}

	int32_t best_fit = -1;
	uint32_t quadrent = 3;
	int32_t distance_from_fit = 0;

	for(size_t i = glyph_details.grapheme_placement.size(); i-- > 0; ) {
		auto& gi = glyph_details.grapheme_placement[i];
		if(gi.line == line) {
			if(gi.x_offset <= xpos && xpos <= gi.x_offset + gi.width) {
				best_fit = int32_t(gi.source_offset);
				if(gi.x_offset + gi.width / 4 < xpos)
					quadrent = 2;
				else if(gi.x_offset + gi.width / 2 < xpos)
					quadrent = 3;
				else if(gi.x_offset + (gi.width * 3) / 4 < xpos) {
					quadrent = 0;
					++best_fit;
				} else {
					quadrent = 1;
					++best_fit;
				}
				break;
			}
			if(xpos < gi.x_offset) {
				if(best_fit == -1 || (gi.x_offset - xpos) < distance_from_fit) {
					best_fit = int32_t(gi.source_offset);
					distance_from_fit = int32_t(gi.x_offset - xpos);
					quadrent = 2;
				}
			}
			if(xpos >= gi.x_offset + gi.width) {
				if(best_fit == -1 || ((xpos + 1) - (gi.x_offset + gi.width)) < distance_from_fit) {
					best_fit = int32_t(gi.source_offset + 1);
					distance_from_fit = int32_t((xpos + 1) - (gi.x_offset + gi.width));
					quadrent = 1;
				}
			}
		}
	}
	return sys::text_mouse_test_result{ uint32_t(best_fit != -1 ? best_fit : 0), quadrent };
}
void edit_box_element_base::edit_move_cursor_to_screen_point(sys::state& state, int32_t x, int32_t y, bool extend_selection) noexcept {
	internal_move_cursor_to_point(state, x, y, extend_selection);

	if(extend_selection == false)
		mouse_entry_position = cursor_position;

	if(glyph_details.grapheme_placement.empty())
		return;

	if(state.ui_state.selecting_edit_text == edit_selection_mode::word) {
		if(cursor_position < mouse_entry_position) {
			cursor_position = std::min(cursor_position, int32_t(glyph_details.grapheme_placement.size()) - 1);
			while(0 <= cursor_position) {
				if(glyph_details.grapheme_placement[cursor_position].is_word_start())
					break;
				--cursor_position;
			}
			anchor_position = mouse_entry_position;
			anchor_position = std::max(anchor_position, 0);
			while(anchor_position < int32_t(glyph_details.grapheme_placement.size())) {
				if(glyph_details.grapheme_placement[anchor_position].is_word_end()) {
					++anchor_position;
					break;
				}
				++anchor_position;
			}
		} else {
			anchor_position = mouse_entry_position;
			anchor_position = std::min(anchor_position, int32_t(glyph_details.grapheme_placement.size()) - 1);
			while(0 <= anchor_position) {
				if(glyph_details.grapheme_placement[anchor_position].is_word_start())
					break;
				--anchor_position;
			}
			cursor_position = std::max(cursor_position, 0);
			while(cursor_position < int32_t(glyph_details.grapheme_placement.size())) {
				if(glyph_details.grapheme_placement[cursor_position].is_word_end()) {
					++cursor_position;
					break;
				}
				++cursor_position;
			}
		}
		internal_on_selection_changed(state);
	} else if(state.ui_state.selecting_edit_text == edit_selection_mode::line) {
		auto cursor_line = glyph_details.grapheme_placement[std::clamp(cursor_position, 0,int32_t(glyph_details.grapheme_placement.size()) -1)].line;
		auto entry_pos_line = glyph_details.grapheme_placement[std::clamp(mouse_entry_position, 0, int32_t(glyph_details.grapheme_placement.size()) - 1)].line;

		if(cursor_position < mouse_entry_position) {
			cursor_position = std::min(cursor_position, int32_t(glyph_details.grapheme_placement.size()) - 1);
			while(0 <= cursor_position) {
				if(glyph_details.grapheme_placement[cursor_position].line != cursor_line) {
					++cursor_position;
					break;
				}
				--cursor_position;
			}
			anchor_position = mouse_entry_position;
			anchor_position = std::max(anchor_position, 0);
			while(anchor_position < int32_t(glyph_details.grapheme_placement.size())) {
				if(glyph_details.grapheme_placement[anchor_position].line != entry_pos_line) {
					break;
				}
				++anchor_position;
			}
		} else {
			anchor_position = mouse_entry_position;
			anchor_position = std::min(anchor_position, int32_t(glyph_details.grapheme_placement.size()) - 1);
			while(0 <= anchor_position) {
				if(glyph_details.grapheme_placement[anchor_position].line != entry_pos_line) {
					++anchor_position;
					break;
				}
				--anchor_position;
			}
			cursor_position = std::max(cursor_position, int32_t(glyph_details.grapheme_placement.size()) - 1);
			while(cursor_position < int32_t(glyph_details.grapheme_placement.size())) {
				if(glyph_details.grapheme_placement[cursor_position].line != cursor_line) {
					break;
				}
				++cursor_position;
			}
		}
	}
	internal_on_selection_changed(state);
}

message_result edit_box_element_base::on_lbutton_down(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mods) noexcept {
	if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) != 0)
		mouse_entry_position = cursor_position;
	state.ui_state.selecting_edit_text = ui::edit_selection_mode::standard;
	internal_move_cursor_to_point(state, x, y, (int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) != 0);
	mouse_entry_position = cursor_position;
	internal_on_selection_changed(state);
	return message_result::consumed;
}

void edit_box_element_base::on_text(sys::state& state, char32_t ch) noexcept {
	if(state.ui_state.edit_target_internal == this && state.ui_state.edit_target_internal->is_visible()) {
		insert_codepoint(state, uint32_t(ch), sys::key_modifiers::modifiers_none);
	}
}

message_result edit_box_element_base::on_key_down(sys::state& state, sys::virtual_key key, sys::key_modifiers mods) noexcept {
	if(state.ui_state.edit_target_internal == this && state.ui_state.edit_target_internal->is_visible()) {
		bool shift_held = (int32_t(mods) & int32_t(sys::key_modifiers::modifiers_shift)) != 0;
		bool ctrl_held = (int32_t(mods) & int32_t(sys::key_modifiers::modifiers_ctrl)) != 0;

		switch(key) {
		case sys::virtual_key::RETURN:
			on_edit_command(state, edit_command::new_line, mods);
			return message_result::consumed;
		case sys::virtual_key::TAB:
			on_edit_command(state, edit_command::tab, mods);
			return message_result::consumed;
		case sys::virtual_key::UP:
			on_edit_command(state, edit_command::cursor_up, mods);
			return message_result::consumed;
		case sys::virtual_key::DOWN:
			on_edit_command(state, edit_command::cursor_down, mods);
			return message_result::consumed;
		case sys::virtual_key::LEFT:
			if(ctrl_held)
				on_edit_command(state, edit_command::cursor_left_word, mods);
			else
				on_edit_command(state, edit_command::cursor_left, mods);
			return message_result::consumed;
		case sys::virtual_key::RIGHT:
			if(ctrl_held)
				on_edit_command(state, edit_command::cursor_right_word, mods);
			else
				on_edit_command(state, edit_command::cursor_right, mods);
			return message_result::consumed;
		case sys::virtual_key::DELETE_KEY:
			if(ctrl_held)
				on_edit_command(state, edit_command::delete_word, mods);
			else
				on_edit_command(state, edit_command::delete_char, mods);
			return message_result::consumed;
		case sys::virtual_key::BACK:
			if(ctrl_held)
				on_edit_command(state, edit_command::backspace_word, mods);
			else
				on_edit_command(state, edit_command::backspace, mods);
			return message_result::consumed;
		case sys::virtual_key::HOME:
			if(ctrl_held)
				on_edit_command(state, edit_command::to_text_start, mods);
			else
				on_edit_command(state, edit_command::to_line_start, mods);
			return message_result::consumed;
		case sys::virtual_key::END:
			if(ctrl_held)
				on_edit_command(state, edit_command::to_text_end, mods);
			else
				on_edit_command(state, edit_command::to_line_end, mods);
			return message_result::consumed;
		case sys::virtual_key::PRIOR:
			on_edit_command(state, edit_command::to_text_start, mods);
			return message_result::consumed;
		case sys::virtual_key::NEXT:
			on_edit_command(state, edit_command::to_text_end, mods);
			return message_result::consumed;
		case sys::virtual_key::INSERT:
			if(ctrl_held)
				on_edit_command(state, edit_command::copy, mods);
			else  if(shift_held)
				on_edit_command(state, edit_command::paste, mods);
			return message_result::consumed;
		case sys::virtual_key::A:
			if(ctrl_held) {
				on_edit_command(state, edit_command::select_all, mods);
				return message_result::consumed;
			}
			return message_result::unseen;
		case sys::virtual_key::X:
			if(ctrl_held) {
				on_edit_command(state, edit_command::cut, mods);
				return message_result::consumed;
			}
			return message_result::unseen;
		case sys::virtual_key::C:
			if(ctrl_held) {
				on_edit_command(state, edit_command::copy, mods);
				return message_result::consumed;
			}
			return message_result::unseen;
		case sys::virtual_key::V:
			if(ctrl_held) {
				on_edit_command(state, edit_command::paste, mods);
				return message_result::consumed;
			}
			return message_result::unseen;
		case sys::virtual_key::Z:
			if(ctrl_held) {
				on_edit_command(state, edit_command::undo, mods);
				return message_result::consumed;
			}
			return message_result::unseen;
		case sys::virtual_key::Y:
			if(ctrl_held) {
				on_edit_command(state, edit_command::redo, mods);
				return message_result::consumed;
			}
			return message_result::unseen;
		default:
			break;
		}
	}
	return message_result::unseen;
}

void edit_box_element_base::on_reset_text(sys::state& state) noexcept {
	internal_on_text_changed(state);
}

void edit_box_element_base::on_create(sys::state& state) noexcept {
	on_reset_text(state);
	ts_obj = state.win_ptr->text_services.create_text_service_object(state, *this);
}

edit_box_element_base::~edit_box_element_base() {
	if(ts_obj)
		window::release_text_services_object(ts_obj);
}

focus_result edit_box_element_base::on_get_focus(sys::state& state) noexcept {
	activation_time = std::chrono::steady_clock::now();
	changes_made = false;
	set_cursor_visibility(state, true);
	if(ts_obj)
		state.win_ptr->text_services.set_focus(state, ts_obj);
	return focus_result::accepted;
}
void edit_box_element_base::set_cursor_visibility(sys::state& state, bool visible) noexcept {
	// TODO: create/destroy system caret
#ifdef _WIN64
	if(visible) {
		uint16_t fonthandle = 0;
		float lineheight = 0;

		if(template_id != -1) {
			alice_ui::grid_size_window* par = static_cast<alice_ui::grid_size_window*>(parent);
			fonthandle = text::make_font_id(state, state.ui_templates.button_t[template_id].primary.font_choice == 1, state.ui_templates.button_t[template_id].primary.font_scale * par->grid_size * 2);
			lineheight = state.font_collection.line_height(state, fonthandle);
		}

		CreateCaret(state.win_ptr->hwnd, nullptr, 1, int32_t(lineheight * state.user_settings.ui_scale));
	} else {
		DestroyCaret();
	}
#endif
}
void edit_box_element_base::on_lose_focus(sys::state& state) noexcept {
	set_cursor_visibility(state, false);
	window::change_cursor(state, window::cursor_type::normal);
	changes_made = false;
}
void edit_box_element_base::on_hover(sys::state& state) noexcept {
	last_activated = std::chrono::steady_clock::now();
	window::change_cursor(state, window::cursor_type::text);
}
void edit_box_element_base::on_hover_end(sys::state& state) noexcept {
	last_activated = std::chrono::steady_clock::now();
	window::change_cursor(state, window::cursor_type::normal);
}
void edit_box_element_base::internal_on_text_changed(sys::state& state) {
	changes_made = true;

	//TODO multiline must save and restore visible line
	if(template_id != -1) {
		glyph_details.grapheme_placement.clear();
		glyph_details.total_lines = 0;

		internal_layout.contents.clear();
		internal_layout.number_of_lines = 0;

		alice_ui::grid_size_window* par = static_cast<alice_ui::grid_size_window*>(parent);
		auto hmargin = int32_t(state.ui_templates.button_t[template_id].primary.h_text_margins * par->grid_size);
		auto al = alice_ui::convert_align(state.ui_templates.button_t[template_id].primary.h_text_alignment);
		auto fh = text::make_font_id(state, state.ui_templates.button_t[template_id].primary.font_choice == 1, state.ui_templates.button_t[template_id].primary.font_scale * par->grid_size * 2);

		text::single_line_layout sl{ internal_layout, text::layout_parameters{ 0, 0, static_cast<int16_t>(base_data.size.x - hmargin * 2), static_cast<int16_t>(base_data.size.y), fh, 0, al, text::text_color::black, true, true }, state.world.locale_get_native_rtl(state.font_collection.get_current_locale()) ? text::layout_base::rtl_status::rtl : text::layout_base::rtl_status::ltr };
		sl.edit_details = &glyph_details;
		sl.add_text(state, cached_text);
	}

	// TODO accessibility integration
	//if(acc_obj && win.is_visible(l_id)) {
	//	win.accessibility_interface.on_text_content_changed(acc_obj);
	//	if(edit_type != edit_contents::number)
	//		win.accessibility_interface.on_text_value_changed(acc_obj);
	//	else
	//		win.accessibility_interface.on_text_numeric_value_changed(acc_obj);
	//}

	edit_box_update(state, cached_text);
	internal_on_selection_changed(state);
}
void edit_box_element_base::internal_on_selection_changed(sys::state& state) {
	// TODO accessibility integration
	// if(acc_obj && win.is_visible(l_id))
	// 	win.accessibility_interface.on_text_selection_changed(acc_obj);

	int32_t hmargin = 0;
	int32_t vmargin = 0;

	if(template_id != -1) {
		alice_ui::grid_size_window* par = static_cast<alice_ui::grid_size_window*>(parent);
		hmargin = int32_t(state.ui_templates.button_t[template_id].primary.h_text_margins * par->grid_size);

		auto fh = text::make_font_id(state, state.ui_templates.button_t[template_id].primary.font_choice == 1, state.ui_templates.button_t[template_id].primary.font_scale * par->grid_size * 2);
		auto linesz = state.font_collection.line_height(state, fh);
		switch(state.ui_templates.button_t[template_id].primary.v_text_alignment) {
		case template_project::aui_text_alignment::center: vmargin = int32_t((base_data.size.y - linesz) / 2); break;
		case template_project::aui_text_alignment::left: vmargin = int32_t(state.ui_templates.button_t[template_id].primary.v_text_margins * par->grid_size);  break;
		case template_project::aui_text_alignment::right: vmargin = int32_t(base_data.size.y - linesz - state.ui_templates.button_t[template_id].primary.v_text_margins * par->grid_size); break;
		}
	}

	if(state.ui_state.edit_target_internal == this) {
#ifdef _WIN64
		auto location = ui::get_absolute_location(state, *this);

		cursor_position = std::max(cursor_position, 0);
		int32_t cursor_x = 0;
		bool rtl = false;
		int32_t line = 0;

		if(glyph_details.grapheme_placement.size() == 0) {
			rtl = state.world.locale_get_native_rtl(state.font_collection.get_current_locale());
			if(rtl)
				cursor_x = base_data.size.x - hmargin * 2;
		} else {
			if(cursor_position < int32_t(glyph_details.grapheme_placement.size())) {
				rtl = glyph_details.grapheme_placement[cursor_position].has_rtl_directionality();
				if(rtl)
					cursor_x = glyph_details.grapheme_placement[cursor_position].x_offset + glyph_details.grapheme_placement[cursor_position].width;
				else
					cursor_x = glyph_details.grapheme_placement[cursor_position].x_offset;
				line = glyph_details.grapheme_placement[cursor_position].line;
			} else {
				rtl = glyph_details.grapheme_placement.back().has_rtl_directionality();
				if(rtl)
					cursor_x = glyph_details.grapheme_placement.back().x_offset;
				else
					cursor_x = glyph_details.grapheme_placement.back().x_offset + glyph_details.grapheme_placement.back().width;
				line = glyph_details.grapheme_placement.back().line;
			}
		}

		auto xpos = float(location.x + hmargin + cursor_x);
		auto ypos = float(location.y + vmargin);

		SetCaretPos(int32_t(xpos), int32_t(ypos));
#endif
	}

	// for multiline
	// make_line_visible(win, text::line_of_position(analysis_obj, cursor_position));

	if(ts_obj) {
		state.win_ptr->text_services.on_selection_change(ts_obj);
	}
}

void edit_box_element_base::insert_codepoint(sys::state& state, uint32_t codepoint, sys::key_modifiers mods) {
	if(disabled)
		return;

	if(!multiline && (codepoint == uint32_t('\n') || codepoint == uint32_t('\r')))
		return;

	auto old_end = std::max(anchor_position, cursor_position);
	auto old_start = std::min(anchor_position, cursor_position);
	if(anchor_position != cursor_position) {
		on_edit_command(state, edit_command::delete_selection, sys::key_modifiers::modifiers_none);
	}
	if(!changes_made)
		edit_undo_buffer.push_state(undo_item{ cached_text, int16_t(anchor_position), int16_t(cursor_position), true });

	auto insert_position = cursor_position < int32_t(glyph_details.grapheme_placement.size()) ?
		std::min(int32_t(glyph_details.grapheme_placement[cursor_position].source_offset), int32_t(cached_text.length()))
		: int32_t(cached_text.length());

	if(codepoint < 0x10000) {
		cached_text.insert(insert_position, 1, uint16_t(codepoint));
		++cursor_position;
	} else {
		auto p = text::make_surrogate_pair(codepoint);
		cached_text.insert(insert_position, 1, uint16_t(p.high));
		cached_text.insert(insert_position + 1, 1, uint16_t(p.low));
		++cursor_position;
	}
	anchor_position = cursor_position;
	internal_on_text_changed(state);
	if(ts_obj) {
		state.win_ptr->text_services.on_text_change(ts_obj, uint32_t(old_start), uint32_t(old_end), uint32_t(cursor_position));
	}
}
void edit_box_element_base::insert_text(sys::state& state, int32_t position_start, int32_t position_end, std::u16string_view content, insertion_source source) noexcept {
	if(state.ui_state.edit_target_internal == this) {
		if(!changes_made)
			edit_undo_buffer.push_state(undo_item{ cached_text, int16_t(anchor_position), int16_t(cursor_position), true });
	}

	auto original_anchor_sp = anchor_position < int32_t(glyph_details.grapheme_placement.size()) ? int32_t(glyph_details.grapheme_placement[std::max(0, anchor_position)].source_offset) : int32_t(cached_text.size());
	auto original_cursor_sp = cursor_position < int32_t(glyph_details.grapheme_placement.size()) ? int32_t(glyph_details.grapheme_placement[std::max(0, cursor_position)].source_offset) : int32_t(cached_text.size());

	if(position_start != position_end)
		cached_text.erase(size_t(position_start), size_t(position_end - position_start));
	cached_text.insert(size_t(position_start), content);

	if(int32_t(position_end) <= original_anchor_sp) {
		original_anchor_sp += int32_t(content.length()) - int32_t(position_end - position_start);
		original_anchor_sp = std::max(0, original_anchor_sp);
	} else if(int32_t(position_start) <= original_anchor_sp && original_anchor_sp <= int32_t(position_end)) {
		original_anchor_sp = int32_t(position_start + content.length());
	}
	if(int32_t(position_end) <= original_cursor_sp) {
		original_cursor_sp += int32_t(content.length()) - int32_t(position_end - position_start);
		original_cursor_sp = std::max(0, original_cursor_sp);
	} else if(int32_t(position_start) <= original_cursor_sp && original_cursor_sp <= int32_t(position_end)) {
		original_cursor_sp = int32_t(position_start + content.length());
	}

	internal_on_text_changed(state);
	set_text_selection(state, original_cursor_sp, original_anchor_sp);

	if(ts_obj && source != insertion_source::text_services) {
		state.win_ptr->text_services.on_text_change(ts_obj, uint32_t(position_start), uint32_t(position_end), uint32_t(position_start + content.length()));
	}
}
bool edit_box_element_base::position_is_ltr(int32_t position) noexcept {
	for(auto j = glyph_details.grapheme_placement.size(); j-- > 0;) {
		if(glyph_details.grapheme_placement[j].source_offset <= position)
			return !glyph_details.grapheme_placement[j].has_rtl_directionality();
	}
	return false;
}
ui::urect edit_box_element_base::text_bounds(sys::state& state, int32_t position_start, int32_t position_end) noexcept {
	int32_t left = 0;
	int32_t top = 0;
	int32_t right = 0;
	int32_t bottom = 0;
	bool first = true;

	uint16_t fonthandle = 0;
	float lineheight = 0;

	if(template_id != -1) {
		alice_ui::grid_size_window* par = static_cast<alice_ui::grid_size_window*>(parent);
		fonthandle = text::make_font_id(state, state.ui_templates.button_t[template_id].primary.font_choice == 1, state.ui_templates.button_t[template_id].primary.font_scale * par->grid_size * 2);
		lineheight = state.font_collection.line_height(state, fonthandle);
	}

	auto abs_pos = ui::get_absolute_location(state, *this);

	for(auto j = glyph_details.grapheme_placement.size(); j-- > 0;) {
		if(position_start <= glyph_details.grapheme_placement[j].source_offset && glyph_details.grapheme_placement[j].source_offset <= position_end) {
			if(first) {
				left = glyph_details.grapheme_placement[j].x_offset;
				right = glyph_details.grapheme_placement[j].x_offset + glyph_details.grapheme_placement[j].width;
				top = int32_t(glyph_details.grapheme_placement[j].line * lineheight);
				bottom = int32_t(glyph_details.grapheme_placement[j].line * lineheight + lineheight);
				first = false;
			} else {
				left = std::min(left, int32_t(glyph_details.grapheme_placement[j].x_offset));
				right = std::max(right, int32_t(glyph_details.grapheme_placement[j].x_offset + glyph_details.grapheme_placement[j].width));
				top = std::min(top, int32_t(glyph_details.grapheme_placement[j].line * lineheight));
				bottom = std::max(bottom, int32_t(glyph_details.grapheme_placement[j].line * lineheight + lineheight));
			}
		}
	}
	if(!first) {
		int32_t hmargin = 0;
		int32_t vmargin = 0;

		if(template_id != -1) {
			alice_ui::grid_size_window* par = static_cast<alice_ui::grid_size_window*>(parent);
			hmargin = int32_t(state.ui_templates.button_t[template_id].primary.h_text_margins * par->grid_size);

			auto fh = text::make_font_id(state, state.ui_templates.button_t[template_id].primary.font_choice == 1, state.ui_templates.button_t[template_id].primary.font_scale * par->grid_size * 2);
			auto linesz = state.font_collection.line_height(state, fh);
			switch(state.ui_templates.button_t[template_id].primary.v_text_alignment) {
			case template_project::aui_text_alignment::center: vmargin = int32_t((base_data.size.y - linesz) / 2); break;
			case template_project::aui_text_alignment::left: vmargin = int32_t(state.ui_templates.button_t[template_id].primary.v_text_margins * par->grid_size);  break;
			case template_project::aui_text_alignment::right: vmargin = int32_t(base_data.size.y - linesz - state.ui_templates.button_t[template_id].primary.v_text_margins * par->grid_size); break;
			}
		}

		return urect{ {int16_t(abs_pos.x + hmargin + left),int16_t(abs_pos.y + vmargin + top)},{int16_t(right - left), int16_t(bottom-top) } };
	} else {
		return urect{ {0,0},{0,0} };
	}
}
int32_t edit_box_element_base::best_cursor_fit_on_line(int32_t line, int32_t xpos) {
	int32_t best_fit = -1;
	int32_t distance_from_fit = 0;
	for(size_t i = glyph_details.grapheme_placement.size(); i-- > 0; ) {
		auto& gi = glyph_details.grapheme_placement[i];
		if(gi.line == line) {
			if(gi.x_offset <= xpos && xpos <= gi.x_offset + gi.width) {
				if(gi.x_offset + gi.width / 2 < xpos)
					return int32_t(i + (gi.has_rtl_directionality() ? 0 : 1));
				else
					return int32_t(i + (gi.has_rtl_directionality() ? 1 : 0));
			}
			if(xpos < gi.x_offset) {
				if(best_fit == -1 || (gi.x_offset - xpos) < distance_from_fit) {
					best_fit = int32_t(i + (gi.has_rtl_directionality() ? 1 : 0));
					distance_from_fit = int32_t(gi.x_offset - xpos);
				}
			}
			if(xpos >= gi.x_offset + gi.width) {
				if(best_fit == -1 || ((xpos + 1) - (gi.x_offset + gi.width)) < distance_from_fit) {
					best_fit = int32_t(i + (gi.has_rtl_directionality() ? 0 : 1));
					distance_from_fit = int32_t((xpos + 1) - (gi.x_offset + gi.width));
				}
			}
		}
	}
	return best_fit;
}
int32_t edit_box_element_base::visually_left_on_line(int32_t line) {
	for(size_t i = glyph_details.grapheme_placement.size(); i-- > 0; ) {
		if(glyph_details.grapheme_placement[i].line == line) {
			while(glyph_details.grapheme_placement[i].visual_left != -1) {
				i = size_t(glyph_details.grapheme_placement[i].visual_left);
			}
			return int32_t(i);
		}
	}
	return -1;
}
int32_t edit_box_element_base::visually_right_on_line(int32_t line) {
	for(size_t i = glyph_details.grapheme_placement.size(); i-- > 0; ) {
		if(glyph_details.grapheme_placement[i].line == line) {
			while(glyph_details.grapheme_placement[i].visual_right != -1) {
				i = size_t(glyph_details.grapheme_placement[i].visual_right);
			}
			return int32_t(i);
		}
	}
	return -1;
}
message_result edit_box_element_base::on_scroll(sys::state& state, int32_t x, int32_t y, float amount, sys::key_modifiers mods) noexcept {
	if(amount >= 1.0f)
		on_edit_command(state, edit_command::cursor_down, mods);
	if(amount <= -1.0f)
		on_edit_command(state, edit_command::cursor_up, mods);
	return message_result::consumed;
}
ui::urect edit_box_element_base::get_edit_bounds(sys::state& state) const {
	// TODO: narrow this to text location?

	auto ui_location = get_absolute_location(state, *this);
	urect result{ ui_location, base_data.size };
	result.top_left.x = int16_t(state.user_settings.ui_scale * result.top_left.x);
	result.top_left.y = int16_t(state.user_settings.ui_scale * result.top_left.y);
	result.size.x = int16_t(state.user_settings.ui_scale * result.size.x);
	result.size.y = int16_t(state.user_settings.ui_scale * result.size.y);

	return result;
}
bool edit_box_element_base::edit_consume_mouse_event(sys::state& state, int32_t x, int32_t y, uint32_t buttons) noexcept {
	auto text_bounds = get_edit_bounds(state);
	if(x < text_bounds.top_left.x || x > text_bounds.top_left.x + text_bounds.size.x || y < text_bounds.top_left.y || y > text_bounds.top_left.y + text_bounds.size.y)
		return false;
	if(ts_obj) {
		return state.win_ptr->text_services.send_mouse_event_to_tso(ts_obj, x, y, buttons);
	}
	return false;
}
void edit_box_element_base::on_edit_command(sys::state& state, edit_command command, sys::key_modifiers mods) noexcept {
	if(disabled)
		return;

	switch(command) {
	case edit_command::new_line:
		if(multiline == false) {
			// nothing
		} else {
			insert_codepoint(state, uint32_t('\n'), mods);
		}
		return;
	case edit_command::backspace:
		if(anchor_position != cursor_position) {
			on_edit_command(state, edit_command::delete_selection, sys::key_modifiers::modifiers_none);
		} else {
			if(!changes_made)
				edit_undo_buffer.push_state(undo_item{ cached_text, int16_t(anchor_position), int16_t(cursor_position), true });
			int32_t to_erase = 0;
			int32_t erase_count = 0;
			if(cursor_position > int32_t(glyph_details.grapheme_placement.size()))
				cursor_position = int32_t(glyph_details.grapheme_placement.size());
			if(cursor_position > 0) {
				to_erase = glyph_details.grapheme_placement[cursor_position - 1].source_offset;
				erase_count = glyph_details.grapheme_placement[cursor_position - 1].unit_length;
			}
			if(to_erase != cursor_position && erase_count > 0) {
				cached_text.erase(size_t(to_erase), size_t(erase_count));
				auto old_cursor = cursor_position;
				cursor_position = to_erase;
				anchor_position = to_erase;
				if(ts_obj) {
					state.win_ptr->text_services.on_text_change(ts_obj, uint32_t(to_erase), uint32_t(to_erase + erase_count), uint32_t(to_erase));
				}
				internal_on_text_changed(state);
			}
		}
		return;
	case edit_command::delete_char:
		if(anchor_position != cursor_position) {
			on_edit_command(state, edit_command::delete_selection, sys::key_modifiers::modifiers_none);
		} else {
			if(!changes_made)
				edit_undo_buffer.push_state(undo_item{ cached_text, int16_t(anchor_position), int16_t(cursor_position), true });
			int32_t to_erase = 0;
			int32_t erase_count = 0;
			if(0 <= cursor_position && cursor_position < int32_t(glyph_details.grapheme_placement.size())) {
				to_erase = glyph_details.grapheme_placement[cursor_position ].source_offset;
				erase_count = glyph_details.grapheme_placement[cursor_position].unit_length;
			}
			if(erase_count != 0) {
				cached_text.erase(size_t(to_erase), size_t(erase_count));
				if(ts_obj) {
					state.win_ptr->text_services.on_text_change(ts_obj, uint32_t(to_erase), uint32_t(to_erase + erase_count), uint32_t(to_erase));
				}
				internal_on_text_changed(state);
			}
		}
		return;
	case edit_command::backspace_word:
		if(anchor_position != cursor_position) {
			on_edit_command(state, edit_command::delete_selection, sys::key_modifiers::modifiers_none);
		} else {
			if(!changes_made)
				edit_undo_buffer.push_state(undo_item{ cached_text, int16_t(anchor_position), int16_t(cursor_position), true });
			int32_t erase_end = 0;
			int32_t erase_start = 0;
			if(cursor_position >= int32_t(glyph_details.grapheme_placement.size())) {
				erase_end = int32_t(cached_text.size());
				cursor_position = int32_t(glyph_details.grapheme_placement.size());
			} else if(0 <= cursor_position) {
				erase_end = glyph_details.grapheme_placement[cursor_position].source_offset;
			}
			while(cursor_position > 0) {
				--cursor_position;
				erase_start = glyph_details.grapheme_placement[cursor_position].source_offset;
				if(glyph_details.grapheme_placement[cursor_position].is_word_start())
					break;
			}
			if(erase_start != erase_end) {
				cached_text.erase(size_t(erase_start), size_t(erase_end - erase_start));
				anchor_position = cursor_position;
				if(ts_obj) {
					state.win_ptr->text_services.on_text_change(ts_obj, uint32_t(erase_start), uint32_t(erase_end), uint32_t(erase_start));
				}
				internal_on_text_changed(state);
			}
		}
		return;
	case edit_command::delete_word:
		if(anchor_position != cursor_position) {
			on_edit_command(state, edit_command::delete_selection, sys::key_modifiers::modifiers_none);
		} else {
			if(!changes_made)
				edit_undo_buffer.push_state(undo_item{ cached_text, int16_t(anchor_position), int16_t(cursor_position), true });

			int32_t erase_start = 0;
			int32_t erase_end = 0;

			if(cursor_position >= int32_t(glyph_details.grapheme_placement.size())) {
				erase_start = int32_t(cached_text.size());
				erase_end = erase_start;
			} else if(cursor_position < 0) {
				cursor_position = 0;
				erase_start = glyph_details.grapheme_placement[cursor_position].source_offset;
				erase_end = erase_start;
			} else {
				erase_start = glyph_details.grapheme_placement[cursor_position].source_offset;
				erase_end = erase_start;
			}

			int32_t temp_cursor_position = cursor_position;
			while(temp_cursor_position < int32_t(glyph_details.grapheme_placement.size())) {
				erase_end += glyph_details.grapheme_placement[temp_cursor_position].unit_length;
				++temp_cursor_position;
			}
			if(erase_start != erase_end) {
				cached_text.erase(size_t(erase_start), size_t(erase_end - erase_start));
				if(ts_obj) {
					state.win_ptr->text_services.on_text_change(ts_obj, uint32_t(erase_start), uint32_t(erase_end), uint32_t(erase_start));
				}
				internal_on_text_changed(state);
			}
		}
		return;
	case edit_command::tab:
		return;
	case edit_command::cursor_down:
	{
		if(glyph_details.grapheme_placement.empty())
			return;
		auto current_pos = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
		auto current_line = glyph_details.grapheme_placement[current_pos].line;
		auto new_pos = best_cursor_fit_on_line(current_line + 1,
			glyph_details.grapheme_placement[current_pos].x_offset + (cursor_position < int32_t(glyph_details.grapheme_placement.size()) ? 0 : glyph_details.grapheme_placement.back().width)
		);
		if(new_pos != -1)
			cursor_position = new_pos;
		else
			cursor_position = int32_t(glyph_details.grapheme_placement.size());
		if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) == 0)
			anchor_position = cursor_position;
		changes_made = false;
		internal_on_selection_changed(state);
	}
		return;
	case edit_command::cursor_up:
	{
		if(glyph_details.grapheme_placement.empty())
			return;
		auto current_pos = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
		auto current_line = glyph_details.grapheme_placement[current_pos].line;
		auto new_pos = best_cursor_fit_on_line(current_line - 1,
			glyph_details.grapheme_placement[current_pos].x_offset + (cursor_position < int32_t(glyph_details.grapheme_placement.size()) ? 0 : glyph_details.grapheme_placement.back().width)
		);
		if(new_pos != -1)
			cursor_position = new_pos;
		else
			cursor_position = 0;
		if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) == 0)
			anchor_position = cursor_position;
		changes_made = false;
		internal_on_selection_changed(state);
	}
		return;
	case edit_command::cursor_left:
	{
		if(glyph_details.grapheme_placement.empty())
			return;

		if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) == 0 && anchor_position != cursor_position) {
			// pick the leftmost end of the selection
			auto c_pos = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
			auto c_line = glyph_details.grapheme_placement[c_pos].line;
			auto a_pos = std::clamp(anchor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
			auto a_line = glyph_details.grapheme_placement[a_pos].line;

			if(c_line != a_line) {
				if(state.world.locale_get_native_rtl(state.font_collection.get_current_locale())) {
					if(c_line > a_line)
						anchor_position = cursor_position;
					else
						cursor_position = anchor_position;
				}
			} else {
				auto c_xpos = glyph_details.grapheme_placement[c_pos].x_offset
					+ (((cursor_position >= int32_t(glyph_details.grapheme_placement.size())) != glyph_details.grapheme_placement[c_pos].has_rtl_directionality()) ? glyph_details.grapheme_placement[c_pos].width : 0);
				auto a_xpos = glyph_details.grapheme_placement[a_pos].x_offset
					+ (((anchor_position >= int32_t(glyph_details.grapheme_placement.size())) != glyph_details.grapheme_placement[a_pos].has_rtl_directionality()) ? glyph_details.grapheme_placement[a_pos].width : 0);
				if(c_xpos <= a_xpos)
					anchor_position = cursor_position;
				else
					cursor_position = anchor_position;
			}
		} else {
			auto current_pos = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
			auto current_line = glyph_details.grapheme_placement[current_pos].line;

			if(cursor_position >= int32_t(glyph_details.grapheme_placement.size())) {
				if(glyph_details.grapheme_placement.back().has_rtl_directionality()) {
					return;
				} else {
					cursor_position = int32_t(glyph_details.grapheme_placement.size() - 1);
				}
			} else if(cursor_position == int32_t(glyph_details.grapheme_placement.size() - 1) && glyph_details.grapheme_placement.back().has_rtl_directionality()) {
				++cursor_position;
			} else {
				auto prev = glyph_details.grapheme_placement[current_pos].visual_left;
				if(prev != -1)
					cursor_position = prev;
				else {
					if(glyph_details.grapheme_placement[current_pos].has_rtl_directionality()) {
						if(auto l = visually_right_on_line(current_line + 1); l != -1)
							cursor_position = 1;
					} else {
						if(auto l = visually_right_on_line(current_line - 1); l != -1)
							cursor_position = 1;
					}
				}
			}
			if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) == 0)
				anchor_position = cursor_position;
		}
		changes_made = false;
		internal_on_selection_changed(state);
	}
		return;
	case edit_command::cursor_right:
	{
		if(glyph_details.grapheme_placement.empty())
			return;

		if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) == 0 && anchor_position != cursor_position) {
			// pick the leftmost end of the selection
			auto c_pos = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
			auto c_line = glyph_details.grapheme_placement[c_pos].line;
			auto a_pos = std::clamp(anchor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
			auto a_line = glyph_details.grapheme_placement[a_pos].line;

			if(c_line != a_line) {
				if(state.world.locale_get_native_rtl(state.font_collection.get_current_locale())) {
					if(c_line < a_line)
						anchor_position = cursor_position;
					else
						cursor_position = anchor_position;
				}
			} else {
				auto c_xpos = glyph_details.grapheme_placement[c_pos].x_offset
					+ (((cursor_position >= int32_t(glyph_details.grapheme_placement.size())) != glyph_details.grapheme_placement[c_pos].has_rtl_directionality()) ? glyph_details.grapheme_placement[c_pos].width : 0);
				auto a_xpos = glyph_details.grapheme_placement[a_pos].x_offset
					+ (((anchor_position >= int32_t(glyph_details.grapheme_placement.size())) != glyph_details.grapheme_placement[a_pos].has_rtl_directionality()) ? glyph_details.grapheme_placement[a_pos].width : 0);
				if(c_xpos >= a_xpos)
					anchor_position = cursor_position;
				else
					cursor_position = anchor_position;
			}
		} else {
			auto current_pos = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
			auto current_line = glyph_details.grapheme_placement[current_pos].line;

			if(cursor_position >= int32_t(glyph_details.grapheme_placement.size())) {
				if(glyph_details.grapheme_placement.back().has_rtl_directionality()) {
					cursor_position = int32_t(glyph_details.grapheme_placement.size() - 1);
				} else {
					return;
				}
			} else if(cursor_position == int32_t(glyph_details.grapheme_placement.size() - 1) && !glyph_details.grapheme_placement.back().has_rtl_directionality()) {
				++cursor_position;
			} else {
				auto next = glyph_details.grapheme_placement[current_pos].visual_right;
				if(next != -1)
					cursor_position = next;
				else {
					if(glyph_details.grapheme_placement[current_pos].has_rtl_directionality()) {
						if(auto l = visually_left_on_line(current_line + 1); l != -1)
							cursor_position = 1;
					} else {
						if(auto l = visually_left_on_line(current_line - 1); l != -1)
							cursor_position = 1;
					}
				}
			}

			if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) == 0)
				anchor_position = cursor_position;
		}
		changes_made = false;
		internal_on_selection_changed(state);
	}
		return;
	case edit_command::cursor_left_word:
	{
		if(glyph_details.grapheme_placement.empty())
			return;
		auto current_pos = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
		bool end_of_word = glyph_details.grapheme_placement[current_pos].has_rtl_directionality();
		bool found = false;
		if(end_of_word) {
			for(size_t i = size_t(cursor_position); i < glyph_details.grapheme_placement.size(); ++i) {
				if(glyph_details.grapheme_placement[i].is_word_end()) {
					cursor_position = int32_t(i + 1);
					found = true;
					break;
				}
			}
			if(!found)
				cursor_position = int32_t(glyph_details.grapheme_placement.size());
		} else {
			for(size_t i = size_t(cursor_position); i-- > 0; ) {
				if(glyph_details.grapheme_placement[i].is_word_start()) {
					cursor_position = int32_t(i);
					found = true;
					break;
				}
			}
			if(!found)
				cursor_position = 0;
		}
		if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) == 0)
			anchor_position = cursor_position;
		changes_made = false;
		internal_on_selection_changed(state);
	}
		return;
	case edit_command::cursor_right_word:
	{
		if(glyph_details.grapheme_placement.empty())
			return;
		auto current_pos = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
		bool end_of_word = !glyph_details.grapheme_placement[current_pos].has_rtl_directionality();
		bool found = false;
		if(end_of_word) {
			for(size_t i = size_t(cursor_position); i < glyph_details.grapheme_placement.size(); ++i) {
				if(glyph_details.grapheme_placement[i].is_word_end()) {
					cursor_position = int32_t(i + 1);
					found = true;
					break;
				}
			}
			if(!found)
				cursor_position = int32_t(glyph_details.grapheme_placement.size());
		} else {
			for(size_t i = size_t(cursor_position); i-- > 0; ) {
				if(glyph_details.grapheme_placement[i].is_word_start()) {
					cursor_position = int32_t(i);
					found = true;
					break;
				}
			}
			if(!found)
				cursor_position = 0;
		}
		if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) == 0)
			anchor_position = cursor_position;
		changes_made = false;
		internal_on_selection_changed(state);
	}
		return;
	case edit_command::to_line_start:
	{
		if(glyph_details.grapheme_placement.empty())
			return;

		auto current_pos = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
		auto current_line = glyph_details.grapheme_placement[current_pos].line;
		for(size_t i = 0; i < glyph_details.grapheme_placement.size(); ++i) {
			if(glyph_details.grapheme_placement[i].line == current_line) {
				cursor_position = int32_t(i);
				break;
			}
		}

		if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) == 0)
			anchor_position = cursor_position;
		changes_made = false;
		internal_on_selection_changed(state);
	}
		return;
	case edit_command::to_line_end:
	{
		if(glyph_details.grapheme_placement.empty())
			return;

		auto current_pos = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
		auto current_line = glyph_details.grapheme_placement[current_pos].line;
		bool changed = false;
		for(size_t i = 0; i < glyph_details.grapheme_placement.size(); ++i) {
			if(glyph_details.grapheme_placement[i].line == current_line + 1) {
				cursor_position = int32_t(i);
				changed = true;
				break;
			}
		}
		if(!changed)
			cursor_position = int32_t(glyph_details.grapheme_placement.size());
		if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) == 0)
			anchor_position = cursor_position;
		changes_made = false;
		internal_on_selection_changed(state);
	}
	return;
	case edit_command::to_text_start:
		cursor_position = 0;
		if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) == 0)
			anchor_position = cursor_position;
		changes_made = false;
		internal_on_selection_changed(state);
		return;
	case edit_command::to_text_end:
		cursor_position = int32_t(glyph_details.grapheme_placement.size());
		if((int32_t(sys::key_modifiers::modifiers_shift) & int32_t(mods)) == 0)
			anchor_position = cursor_position;
		changes_made = false;
		internal_on_selection_changed(state);
		return;
	case edit_command::cut:
		if(anchor_position != cursor_position) {
			on_edit_command(state, edit_command::copy, sys::key_modifiers::modifiers_none);
			on_edit_command(state, edit_command::delete_selection, sys::key_modifiers::modifiers_none);
		}
		return;
	case edit_command::copy:
		if(anchor_position != cursor_position) {
#ifdef _WIN64
			if(OpenClipboard(state.win_ptr->hwnd)) {
				if(EmptyClipboard()) {
					cursor_position = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size()));
					anchor_position = std::clamp(anchor_position, 0, int32_t(glyph_details.grapheme_placement.size()));
					auto start_c = std::min(anchor_position, cursor_position);
					auto end_c = std::max(anchor_position, cursor_position);
					auto start = (size_t(start_c) < glyph_details.grapheme_placement.size()) ? size_t(glyph_details.grapheme_placement[start_c].source_offset) : cached_text.size();
					auto end = (size_t(end_c) < glyph_details.grapheme_placement.size()) ? size_t(glyph_details.grapheme_placement[end_c].source_offset) : cached_text.size();

					size_t byteSize = sizeof(char16_t) * ((end - start) + 1);
					HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE | GMEM_ZEROINIT, byteSize);

					if(hClipboardData != nullptr) {
						void* memory = GlobalLock(hClipboardData);

						if(memory != nullptr) {
							memcpy(memory, cached_text.data() + start, sizeof(char16_t)* (end - start));
							memset((char*)memory + sizeof(char16_t) * (end - start), 0, sizeof(char16_t));
							GlobalUnlock(hClipboardData);

							if(SetClipboardData(CF_UNICODETEXT, hClipboardData) != nullptr) {
								hClipboardData = nullptr; // system now owns the clipboard, so don't touch it.
							}
						}
						GlobalFree(hClipboardData); // free if failed
					}
				}
				CloseClipboard();
			}
#endif
			changes_made = false;
		}
		return;
	case edit_command::paste:
	{
		auto old_start_position = std::min(anchor_position, cursor_position);
		auto old_end_position = std::max(anchor_position, cursor_position);
		edit_undo_buffer.push_state(undo_item{ cached_text, int16_t(anchor_position), int16_t(cursor_position), true });
		bool temp_change_made = false;

		if(anchor_position != cursor_position) {
			auto start = std::min(anchor_position, cursor_position);
			auto length = std::max(anchor_position, cursor_position) - start;
			cached_text.erase(size_t(start), size_t(length));
			cursor_position = start;
			anchor_position = start;
			temp_change_made = true;
		}

		std::u16string temp_text;
#ifdef _WIN64
		if(OpenClipboard(state.win_ptr->hwnd)) {
			HGLOBAL hClipboardData = GetClipboardData(CF_UNICODETEXT);

			if(hClipboardData != NULL) {
				size_t byteSize = GlobalSize(hClipboardData);
				void* memory = GlobalLock(hClipboardData);
				if(memory != NULL) {
					const char16_t* text = reinterpret_cast<const char16_t*>(memory);
					temp_text = std::u16string(text, text + byteSize / sizeof(char16_t));
					GlobalUnlock(hClipboardData);
					if(temp_text.length() > 0 && temp_text.back() == 0) {
						temp_text.pop_back();
					}
					if(multiline == false) {
						for(auto& ch : temp_text) {
							if(ch == char16_t('\n') || ch == char16_t('\r'))
								ch = char16_t(' ');
						}
					}
				}
			}
			CloseClipboard();
		}
#endif
		bool cursor_needs_updating = false;
		if(temp_text.size() > 0) {
			auto old_source_pos = (0 <= cursor_position && cursor_position < int32_t(glyph_details.grapheme_placement.size())) ? size_t(glyph_details.grapheme_placement[cursor_position].source_offset) : cached_text.length();
			auto new_source_pos = old_source_pos + temp_text.length();

			cached_text.insert(size_t(cursor_position), temp_text);
			temp_change_made = true;
			internal_on_text_changed(state);

			cursor_position = 0;
			for(; size_t(cursor_position) < glyph_details.grapheme_placement.size(); ++cursor_position) {
				if(glyph_details.grapheme_placement[cursor_position].source_offset >= new_source_pos)
					break;
			}
			anchor_position = cursor_position;
		}

		if(ts_obj && temp_change_made) {
			state.win_ptr->text_services.on_text_change(ts_obj, uint32_t(old_start_position), uint32_t(old_end_position), uint32_t(cursor_position));
		}
		changes_made = false;
	}
	return;
	case edit_command::select_all:
		anchor_position = 0;
		cursor_position = int32_t(glyph_details.grapheme_placement.size());
		internal_on_selection_changed(state);
		return;
	case edit_command::undo:
	{
		auto undostate = edit_undo_buffer.undo(undo_item{ cached_text, int16_t(anchor_position), int16_t(cursor_position), true });
		if(undostate.has_value()) {
			auto old_length = cached_text.length();
			cached_text = (*undostate).contents;
			cursor_position = (*undostate).cursor;
			anchor_position = (*undostate).anchor;

			internal_on_text_changed(state);

			if(ts_obj) {
				state.win_ptr->text_services.on_text_change(ts_obj, uint32_t(0), uint32_t(old_length), uint32_t(cached_text.length()));
			}
		}
	}
	return;
	case edit_command::redo:
	{
		auto redostate = edit_undo_buffer.redo(undo_item{ cached_text, int16_t(anchor_position), int16_t(cursor_position), true });
		if(redostate.has_value()) {
			auto old_length = cached_text.length();
			cached_text = (*redostate).contents;
			cursor_position = (*redostate).cursor;
			anchor_position = (*redostate).anchor;

			internal_on_text_changed(state);

			if(ts_obj) {
				state.win_ptr->text_services.on_text_change(ts_obj, uint32_t(0), uint32_t(old_length), uint32_t(cached_text.length()));
			}
		}
	}
	return;
	case edit_command::select_current_word:
	{
		if(glyph_details.grapheme_placement.empty())
			return;

		anchor_position = cursor_position;
		bool found = false;
		for(size_t i = size_t(cursor_position); i < glyph_details.grapheme_placement.size(); ++i) {
			if(glyph_details.grapheme_placement[i].is_word_end()) {
				cursor_position = int32_t(i + 1);
				found = true;
				break;
			}
		}
		if(!found)
			cursor_position = int32_t(glyph_details.grapheme_placement.size());
		found = false;

		for(size_t i = size_t(anchor_position); i-- > 0; ) {
			if(glyph_details.grapheme_placement[i].is_word_start()) {
				anchor_position = int32_t(i);
				found = true;
				break;
			}
		}
		if(!found)
			anchor_position = 0;

		internal_on_selection_changed(state);
		changes_made = false;
	}
		return;
	case edit_command::select_current_section:
	{
		if(glyph_details.grapheme_placement.empty())
			return;

		auto current_pos = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size() - 1));
		auto current_line = glyph_details.grapheme_placement[current_pos].line;
		anchor_position = cursor_position;

		bool end_found = false;
		bool start_found = false;

		for(size_t i = 0; i < glyph_details.grapheme_placement.size(); ++i) {
			if(!start_found && glyph_details.grapheme_placement[i].line == current_line) {
				anchor_position = int32_t(i);
				start_found = true;
			}
			if(glyph_details.grapheme_placement[i].line == current_line + 1) {
				cursor_position = int32_t(i);
				end_found = true;
				break;
			}
		}
		if(start_found && !end_found) {
			cursor_position = int32_t(glyph_details.grapheme_placement.size());
		}
		internal_on_selection_changed(state);
		changes_made = false;
	}
		return;
	case edit_command::delete_selection:
		if(anchor_position != cursor_position) {
			edit_undo_buffer.push_state(undo_item{ cached_text, int16_t(anchor_position), int16_t(cursor_position), true });

			cursor_position = std::clamp(cursor_position, 0, int32_t(glyph_details.grapheme_placement.size()));
			anchor_position = std::clamp(anchor_position, 0, int32_t(glyph_details.grapheme_placement.size()));
			auto start_c = std::min(anchor_position, cursor_position);
			auto end_c = std::max(anchor_position, cursor_position);
			auto start = (size_t(start_c) < glyph_details.grapheme_placement.size()) ? size_t(glyph_details.grapheme_placement[start_c].source_offset) : cached_text.size();
			auto end = (size_t(end_c) < glyph_details.grapheme_placement.size()) ? size_t(glyph_details.grapheme_placement[end_c].source_offset) : cached_text.size();
			cached_text.erase(start, end-start);

			cursor_position = int32_t(start_c);
			anchor_position = int32_t(start_c);

			internal_on_text_changed(state);

			if(ts_obj) {
				state.win_ptr->text_services.on_text_change(ts_obj, uint32_t(start), uint32_t(end), uint32_t(start));
			}
		}
		return;
	}
}
void edit_box_element_base::set_text(sys::state& state, std::u16string const& new_text) {
	if(template_id != -1) {
		if(new_text != cached_text) {
			alice_ui::grid_size_window* par = static_cast<alice_ui::grid_size_window*>(parent);
			auto hmargin = state.ui_templates.button_t[template_id].primary.h_text_margins * par->grid_size;

			glyph_details.grapheme_placement.clear();
			glyph_details.total_lines = 0;

			if(!changes_made)
				edit_undo_buffer.push_state(undo_item{ cached_text, int16_t(anchor_position), int16_t(cursor_position), true });

			cached_text = new_text;
			{
				internal_layout.contents.clear();
				internal_layout.number_of_lines = 0;

				auto al = alice_ui::convert_align(state.ui_templates.button_t[template_id].primary.h_text_alignment);
				auto fonthandle = text::make_font_id(state, state.ui_templates.button_t[template_id].primary.font_choice == 1, state.ui_templates.button_t[template_id].primary.font_scale * par->grid_size * 2);
				text::single_line_layout sl{ internal_layout, text::layout_parameters{ 0, 0, static_cast<int16_t>(base_data.size.x - hmargin* 2), static_cast<int16_t>(base_data.size.y),
							fonthandle, 0, al, text::text_color::black, true, true },
					state.world.locale_get_native_rtl(state.font_collection.get_current_locale()) ? text::layout_base::rtl_status::rtl : text::layout_base::rtl_status::ltr };
				sl.edit_details = &glyph_details;
				sl.add_text(state, cached_text);
			}
		}
	}

	cursor_position = int32_t(glyph_details.grapheme_placement.size());
	anchor_position = cursor_position;
}
void edit_box_element_base::render(sys::state& state, int32_t x, int32_t y) noexcept {
	float hmargin = 0;
	float vmargin = 0;
	ogl::color3f base_color;
	uint16_t fonthandle = 0;
	float lineheight =0;

	if(template_id != -1) {
		template_project::text_region_template region;
		alice_ui::grid_size_window* par = static_cast<alice_ui::grid_size_window*>(parent);

		auto ms_after = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_activated);
		if(disabled) {
			region = state.ui_templates.button_t[template_id].disabled;
			auto bg_id = region.bg;
			if(bg_id != -1) {
				ogl::render_textured_rect_direct(state, float(x), float(y), float(base_data.size.x), float(base_data.size.y),
					state.ui_templates.backgrounds[bg_id].renders.get_render(state, float(base_data.size.x) / float(par->grid_size), float(base_data.size.y) / float(par->grid_size), int32_t(par->grid_size), state.user_settings.ui_scale));
			}
		} else if(ms_after.count() < alice_ui::mouse_over_animation_ms && state.ui_templates.button_t[template_id].animate_active_transition) {
			float percentage = float(ms_after.count()) / float(alice_ui::mouse_over_animation_ms);
			if(this == state.ui_state.under_mouse) {
				region = state.ui_templates.button_t[template_id].active;
				auto active_id = state.ui_templates.button_t[template_id].active.bg;
				if(active_id != -1) {
					ogl::render_rect_slice(state, float(x), float(y), float(base_data.size.x), float(base_data.size.y),
						state.ui_templates.backgrounds[active_id].renders.get_render(state, float(base_data.size.x) / float(par->grid_size), float(base_data.size.y) / float(par->grid_size), int32_t(par->grid_size), state.user_settings.ui_scale),
						0.0f, percentage);
				}
				auto bg_id = state.ui_templates.button_t[template_id].primary.bg;
				if(bg_id != -1) {
					ogl::render_rect_slice(state, float(x), float(y), float(base_data.size.x), float(base_data.size.y),
						state.ui_templates.backgrounds[bg_id].renders.get_render(state, float(base_data.size.x) / float(par->grid_size), float(base_data.size.y) / float(par->grid_size), int32_t(par->grid_size), state.user_settings.ui_scale),
						percentage, 1.0f);
				}

			} else {
				region = state.ui_templates.button_t[template_id].primary;
				auto active_id = state.ui_templates.button_t[template_id].active.bg;
				if(active_id != -1) {
					ogl::render_rect_slice(state, float(x), float(y), float(base_data.size.x), float(base_data.size.y),
						state.ui_templates.backgrounds[active_id].renders.get_render(state, float(base_data.size.x) / float(par->grid_size), float(base_data.size.y) / float(par->grid_size), int32_t(par->grid_size), state.user_settings.ui_scale),
						percentage, 1.0f);
				}
				auto bg_id = state.ui_templates.button_t[template_id].primary.bg;
				if(bg_id != -1) {
					ogl::render_rect_slice(state, float(x), float(y), float(base_data.size.x), float(base_data.size.y),
						state.ui_templates.backgrounds[bg_id].renders.get_render(state, float(base_data.size.x) / float(par->grid_size), float(base_data.size.y) / float(par->grid_size), int32_t(par->grid_size), state.user_settings.ui_scale),
						0.0f, percentage);
				}
			}
		} else if(this == state.ui_state.under_mouse) {
			region = state.ui_templates.button_t[template_id].active;
			auto active_id = region.bg;
			if(active_id != -1) {
				ogl::render_textured_rect_direct(state, float(x), float(y), float(base_data.size.x), float(base_data.size.y),
					state.ui_templates.backgrounds[active_id].renders.get_render(state, float(base_data.size.x) / float(par->grid_size), float(base_data.size.y) / float(par->grid_size), int32_t(par->grid_size), state.user_settings.ui_scale));
			}

		} else {
			region = state.ui_templates.button_t[template_id].primary;
			auto bg_id = region.bg;
			if(bg_id != -1) {
				ogl::render_textured_rect_direct(state, float(x), float(y), float(base_data.size.x), float(base_data.size.y),
					state.ui_templates.backgrounds[bg_id].renders.get_render(state, float(base_data.size.x) / float(par->grid_size), float(base_data.size.y) / float(par->grid_size), int32_t(par->grid_size), state.user_settings.ui_scale));
			}
		}

		auto color = state.ui_templates.colors[region.text_color];

		fonthandle = text::make_font_id(state, region.font_choice == 1, region.font_scale * par->grid_size * 2);
		lineheight = state.font_collection.line_height(state, fonthandle);
		if(lineheight == 0.f)
			return;

		int32_t yoff = 0;
		switch(region.v_text_alignment) {
		case template_project::aui_text_alignment::center: yoff = int32_t((base_data.size.y - lineheight) / 2); break;
		case template_project::aui_text_alignment::left: yoff = int32_t(region.v_text_margins * par->grid_size);  break;
		case template_project::aui_text_alignment::right: yoff = int32_t(base_data.size.y - lineheight - region.v_text_margins * par->grid_size); break;
		}

		for(auto& t : internal_layout.contents) {
			ui::render_text_chunk(
				state,
				t,
				float(x) + t.x + region.h_text_margins * par->grid_size,
				float(y + t.y + yoff),
				fonthandle,
				ogl::color3f{ color.r, color.g, color.b },
				ogl::color_modification::none
			);
		}
		hmargin = float(region.h_text_margins * par->grid_size);
		vmargin = float(yoff);
	}

	if(disabled)
		return;

	ogl::color3f invert_color{ 1.0f - base_color.r, 1.0f - base_color.g, 1.0f - base_color.b };

	// render selection
	if(cursor_position != anchor_position) {
		auto start = std::clamp(std::min(cursor_position, anchor_position), 0, int32_t(glyph_details.grapheme_placement.size()));
		auto end = std::clamp(std::max(cursor_position, anchor_position), 0, int32_t(glyph_details.grapheme_placement.size()));

		bool rtl_chunk = false;
		int32_t min_x = 0;
		int32_t max_x = 0;
		int32_t line = 0;

		if(start < int32_t(glyph_details.grapheme_placement.size())) {
			rtl_chunk = glyph_details.grapheme_placement[start].has_rtl_directionality();
			min_x = glyph_details.grapheme_placement[start].x_offset + (rtl_chunk ? glyph_details.grapheme_placement[start].width : 0);
			max_x = glyph_details.grapheme_placement[start].x_offset + (rtl_chunk ? glyph_details.grapheme_placement[start].width : 0);
			line = glyph_details.grapheme_placement[start].line;
		}

		while(start <= end) {
			if(start == end || glyph_details.grapheme_placement[start].line != line || rtl_chunk != glyph_details.grapheme_placement[start].has_rtl_directionality()) {
				// new run reached -- render highlight for old run
				if(min_x != max_x) {
					ogl::scissor_box bounds{
						state,
						int32_t(x + hmargin + min_x + 1),
						int32_t(y + vmargin + lineheight * line),
						max_x - min_x,
						int32_t(lineheight + 1)
					};

					ogl::render_alpha_colored_rect(state,
						float(x + hmargin + min_x + 1),
						y + vmargin + lineheight * line,
						float(max_x - min_x), float(lineheight + 1),
						base_color.r, base_color.g, base_color.b, 1.0f);

					for(auto& t : internal_layout.contents) {
						render_text_chunk(
							state, t,
							float(x + hmargin) + t.x,
							float(y + vmargin),
							fonthandle, invert_color, ogl::color_modification::none
						);
					}
				}
				if(start != end) { // setup start of next run
					rtl_chunk = glyph_details.grapheme_placement[start].has_rtl_directionality();
					min_x = glyph_details.grapheme_placement[start].x_offset + (rtl_chunk ? glyph_details.grapheme_placement[start].width : 0);
					max_x = glyph_details.grapheme_placement[start].x_offset + (rtl_chunk ? glyph_details.grapheme_placement[start].width : 0);
					line = glyph_details.grapheme_placement[start].line;
				} else {
					break;
				}
			} else { // continue chunk
				min_x = std::min(min_x, glyph_details.grapheme_placement[start].x_offset + (rtl_chunk ? 0 : glyph_details.grapheme_placement[start].width));
				max_x = std::max(max_x, glyph_details.grapheme_placement[start].x_offset + (rtl_chunk ? 0 : glyph_details.grapheme_placement[start].width));
				++start;
			}
		}
	}
	// repeat logic for highlighting the composition text
	if(temp_selection_start != temp_selection_end) {
		// find the exgc index
		auto gcstart = int32_t(glyph_details.grapheme_placement.size());
		for(int32_t j = 0; j < int32_t(glyph_details.grapheme_placement.size()); ++j) {
			if(glyph_details.grapheme_placement[j].source_offset == temp_selection_start) {
				gcstart = j;
				break;
			}
		}
		auto gcend = int32_t(glyph_details.grapheme_placement.size());
		for(int32_t j = 0; j < int32_t(glyph_details.grapheme_placement.size()); ++j) {
			if(glyph_details.grapheme_placement[j].source_offset == temp_selection_end) {
				gcend = j;
				break;
			}
		}
		auto start = std::clamp(std::min(gcstart, gcend), 0, int32_t(glyph_details.grapheme_placement.size()));
		auto end = std::clamp(std::max(gcstart, gcend), 0, int32_t(glyph_details.grapheme_placement.size()));


		bool rtl_chunk = false;
		int32_t min_x = 0;
		int32_t max_x = 0;
		int32_t line = 0;

		if(start < int32_t(glyph_details.grapheme_placement.size())) {
			rtl_chunk = glyph_details.grapheme_placement[start].has_rtl_directionality();
			min_x = glyph_details.grapheme_placement[start].x_offset + (rtl_chunk ? glyph_details.grapheme_placement[start].width : 0);
			max_x = glyph_details.grapheme_placement[start].x_offset + (rtl_chunk ? glyph_details.grapheme_placement[start].width : 0);
			line = glyph_details.grapheme_placement[start].line;
		}

		while(start <= end) {
			if(start == end || glyph_details.grapheme_placement[start].line != line || rtl_chunk != glyph_details.grapheme_placement[start].has_rtl_directionality()) {
				// new run reached -- render highlight for old run
				if(min_x != max_x) {
					ogl::scissor_box bounds{
						state,
						int32_t(x + hmargin + min_x + 1),
						int32_t(y + vmargin + lineheight * line),
						max_x - min_x,
						int32_t(lineheight + 1)
					};

					ogl::render_alpha_colored_rect(state,
						float(x + hmargin + min_x + 1),
						y + vmargin + lineheight * line,
						float(max_x - min_x), float(lineheight + 1),
						base_color.r, base_color.g, base_color.b, 1.0f);

					for(auto& t : internal_layout.contents) {
						render_text_chunk(
							state, t,
							float(x + hmargin) + t.x,
							float(y + vmargin),
							fonthandle, invert_color, ogl::color_modification::none
						);
					}
				}
				if(start != end) { // setup start of next run
					rtl_chunk = glyph_details.grapheme_placement[start].has_rtl_directionality();
					min_x = glyph_details.grapheme_placement[start].x_offset + (rtl_chunk ? glyph_details.grapheme_placement[start].width : 0);
					max_x = glyph_details.grapheme_placement[start].x_offset + (rtl_chunk ? glyph_details.grapheme_placement[start].width : 0);
					line = glyph_details.grapheme_placement[start].line;
				} else {
					break;
				}
			} else { // continue chunk
				min_x = std::min(min_x, glyph_details.grapheme_placement[start].x_offset + (rtl_chunk ? 0 : glyph_details.grapheme_placement[start].width));
				max_x = std::max(max_x, glyph_details.grapheme_placement[start].x_offset + (rtl_chunk ? 0 : glyph_details.grapheme_placement[start].width));
				++start;
			}
		}
	}
	if(state.ui_state.edit_target_internal == this) {
		auto this_time = std::chrono::steady_clock::now();
		auto ms_count = std::chrono::duration_cast<std::chrono::milliseconds>(this_time - activation_time).count();
		if(cursor_position > int32_t(glyph_details.grapheme_placement.size())) {
			cursor_position = int32_t(glyph_details.grapheme_placement.size());
		}
		cursor_position = std::max(cursor_position, 0);
		int32_t cursor_x = 0;
		bool rtl = false;
		int32_t line = 0;

		if(glyph_details.grapheme_placement.size() == 0) {
			rtl = state.world.locale_get_native_rtl(state.font_collection.get_current_locale());
			if(rtl)
				cursor_x = int32_t(base_data.size.x - hmargin);
		} else {
			if(cursor_position < int32_t(glyph_details.grapheme_placement.size())) {
				rtl = glyph_details.grapheme_placement[cursor_position].has_rtl_directionality();
				if(rtl)
					cursor_x = glyph_details.grapheme_placement[cursor_position].x_offset + glyph_details.grapheme_placement[cursor_position].width;
				else
					cursor_x = glyph_details.grapheme_placement[cursor_position].x_offset;
				line = glyph_details.grapheme_placement[cursor_position].line;
			} else {
				rtl = glyph_details.grapheme_placement[cursor_position - 1].has_rtl_directionality();
				if(rtl)
					cursor_x = glyph_details.grapheme_placement[cursor_position - 1].x_offset;
				else
					cursor_x = glyph_details.grapheme_placement[cursor_position - 1].x_offset + glyph_details.grapheme_placement[cursor_position - 1].width;
				line = glyph_details.grapheme_placement[cursor_position - 1].line;
			}
		}

		auto xpos = float(x + hmargin + cursor_x);
		auto ypos = float(y + vmargin);


		auto alpha = window::cursor_blink_ms() > 0 ? (std::cos(float(ms_count % window::cursor_blink_ms()) /  float(window::cursor_blink_ms()) * 2.0f * 3.14159f) + 1.0f) / 2.0f : 1.0f;
		if(anchor_position != cursor_position)
			ogl::render_alpha_colored_rect(state, xpos, ypos, 1, lineheight, invert_color.r, invert_color.g, invert_color.b, float(1.0f - alpha));
		ogl::render_alpha_colored_rect(state, xpos, ypos, 1, lineheight, base_color.r, base_color.g, base_color.b, float(alpha));
		if(rtl) {
			if(anchor_position != cursor_position)
				ogl::render_alpha_colored_rect(state, xpos + 1, ypos + lineheight - 1, 2, 1, invert_color.r, invert_color.g, invert_color.b, float(1.0f - alpha));
			ogl::render_alpha_colored_rect(state, xpos + 1, ypos + lineheight - 1, 2, 1, base_color.r, base_color.g, base_color.b, float(alpha));
		}
	}
}

void tool_tip::render(sys::state& state, int32_t x, int32_t y) noexcept {
	static auto popup_bg = template_project::background_by_name(state.ui_templates, "outset_region.asvg");
	static auto ink_color = template_project::color_by_name(state.ui_templates, "ink");

	ogl::render_textured_rect_direct(state, float(x), float(y), float(base_data.size.x),
		float(base_data.size.y), state.ui_templates.backgrounds[popup_bg].renders.get_render(state, float(base_data.size.x) / float(9), float(base_data.size.y) / float(9), int32_t(9), state.user_settings.ui_scale));


	for(auto& t : internal_layout.contents) {
		auto& f = state.font_collection.get_font(state, text::font_index_from_font_id(state.ui_state.default_body_font));
		render_text_chunk(
			state,
			t,
			float(x) + t.x,
			float(y + t.y),
			state.ui_state.default_body_font,
			state.ui_templates.colors[ink_color],
			ogl::color_modification::none
		);
	}
}

state::state() {
	root = std::make_unique<container_base>();
	tooltip = std::make_unique<tool_tip>();
	tooltip->flags |= element_base::is_invisible_mask;
}

state::~state() = default;



} // namespace ui
