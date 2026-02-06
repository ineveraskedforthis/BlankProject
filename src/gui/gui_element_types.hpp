#pragma once

#include "dcon_generated_ids.hpp"
#include "gui_graphics.hpp"
#include "gui_element_base.hpp"
#include "opengl_wrapper.hpp"
#include "sound.hpp"
#include "system_state_forward.hpp"
#include "text.hpp"
#include "texture.hpp"
#include <cstdint>
#include <vector>

namespace window {
struct text_services_object;
}

namespace ui {



class container_base : public element_base {
public:
	std::vector<std::unique_ptr<element_base>> children;

	mouse_probe impl_probe_mouse(sys::state& state, int32_t x, int32_t y, mouse_probe_type type) noexcept override;
	message_result impl_on_key_down(sys::state& state, sys::virtual_key key, sys::key_modifiers mods) noexcept final;
	void impl_on_update(sys::state& state) noexcept override;

	void impl_render(sys::state& state, int32_t x, int32_t y) noexcept override;
	void impl_on_reset_text(sys::state& state) noexcept override;
	drag_and_drop_query_result impl_drag_and_drop_query(sys::state& state, int32_t x, int32_t y, ui::drag_and_drop_data data_type) noexcept override;

	std::unique_ptr<element_base> remove_child(element_base* child) noexcept final;
	void move_child_to_front(element_base* child) noexcept final;
	void move_child_to_back(element_base* child) noexcept final;
	void add_child_to_front(std::unique_ptr<element_base> child) noexcept final;
	void add_child_to_back(std::unique_ptr<element_base> child) noexcept final;
	element_base* get_child_by_index(sys::state const& state, int32_t index) noexcept final;
};

class non_owning_container_base : public element_base {
public:
	std::vector<element_base*> children;

	mouse_probe impl_probe_mouse(sys::state& state, int32_t x, int32_t y, mouse_probe_type type) noexcept override;
	message_result impl_on_key_down(sys::state& state, sys::virtual_key key, sys::key_modifiers mods) noexcept final;
	void impl_on_update(sys::state& state) noexcept override;

	void impl_render(sys::state& state, int32_t x, int32_t y) noexcept override;
	void impl_on_reset_text(sys::state& state) noexcept override;
	drag_and_drop_query_result impl_drag_and_drop_query(sys::state& state, int32_t x, int32_t y, ui::drag_and_drop_data data_type) noexcept override;

	void move_child_to_front(element_base* child) noexcept final;
	void move_child_to_back(element_base* child) noexcept final;
	element_base* get_child_by_index(sys::state const& state, int32_t index) noexcept final;
};


class edit_box_element_base : public element_base {
protected:
	struct undo_item {
		std::u16string contents;
		int16_t anchor = 0;
		int16_t cursor = 0;
		bool valid = false;

		bool operator==(undo_item const& o) const noexcept {
			return contents == o.contents;
		}
		bool operator!=(undo_item const& o) const noexcept {
			return !(*this == o);
		}
	};

	struct undo_buffer {
		constexpr static int32_t total_size = 16;

		undo_item interal_buffer[total_size] = {};
		int32_t buffer_position = 0;

		std::optional<undo_item> undo(undo_item current_state) {
			push_state(current_state);

			auto temp_back = buffer_position - 1;
			if(temp_back < 0) {
				temp_back += total_size;
			}

			if(interal_buffer[temp_back].valid) {
				buffer_position = temp_back;
				return interal_buffer[temp_back];
			}
			return std::optional<undo_item>{};
		}
		std::optional<undo_item> redo(undo_item current_state) {
			if(interal_buffer[buffer_position] == current_state) {
				auto temp_back = buffer_position + 1;
				if(temp_back >= total_size) {
					temp_back -= total_size;
				}
				if(interal_buffer[temp_back].valid) {
					buffer_position = temp_back;
					return interal_buffer[buffer_position];
				}
			}
			return std::optional<undo_item>{};
		}
		void push_state(undo_item state) {
			if(interal_buffer[buffer_position].valid == false || interal_buffer[buffer_position] != state) {
				++buffer_position;
				if(buffer_position >= total_size) {
					buffer_position -= total_size;
				}
				interal_buffer[buffer_position] = state;
				interal_buffer[buffer_position].valid = true;

				auto temp_next = buffer_position + 1;
				if(temp_next >= total_size) {
					temp_next -= total_size;
				}
				interal_buffer[temp_next].valid = false;
			}
		}
	} edit_undo_buffer;

	std::u16string cached_text;
	text::layout internal_layout;

	std::u16string_view get_text(sys::state& state) const {
		return cached_text;
	}

	text::layout_details glyph_details;
	std::chrono::time_point<std::chrono::steady_clock> activation_time;
	window::text_services_object* ts_obj = nullptr;

	int32_t cursor_position = 0;
	int32_t anchor_position = 0;
	int32_t mouse_entry_position = 0;
	int32_t temp_selection_start = 0;
	int32_t temp_selection_end = 0;
	std::chrono::steady_clock::time_point last_activated;

	bool multiline = false;
	bool changes_made = false;

	void insert_codepoint(sys::state& state, uint32_t codepoint, sys::key_modifiers mods);
	void internal_on_text_changed(sys::state& state);
	void internal_on_selection_changed(sys::state& state);
	void internal_move_cursor_to_point(sys::state& state, int32_t x, int32_t y, bool extend_selection);
	int32_t best_cursor_fit_on_line(int32_t line, int32_t xpos);
	int32_t visually_left_on_line(int32_t line);
	int32_t visually_right_on_line(int32_t line);
	ui::urect get_edit_bounds(sys::state& state) const;
public:
	int32_t template_id = -1;
	bool disabled = false;

	void set_text(sys::state& state, std::u16string const& new_text);

	virtual void edit_box_update(sys::state& state, std::u16string_view s) noexcept { }

	void on_reset_text(sys::state& state) noexcept override;
	void on_create(sys::state& state) noexcept override;

	message_result test_mouse(sys::state& state, int32_t x, int32_t y, mouse_probe_type type) noexcept override {
		return message_result::consumed;
	}
	message_result on_lbutton_down(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mods) noexcept override;
	message_result on_key_down(sys::state& state, sys::virtual_key key, sys::key_modifiers mods) noexcept override;
	void on_text(sys::state& state, char32_t ch) noexcept override;
	void on_edit_command(sys::state& state, edit_command command, sys::key_modifiers mods) noexcept override;
	bool edit_consume_mouse_event(sys::state& state, int32_t x, int32_t y, uint32_t buttons) noexcept override;
	message_result on_scroll(sys::state& state, int32_t x, int32_t y, float amount, sys::key_modifiers mods) noexcept override;
	void render(sys::state& state, int32_t x, int32_t y) noexcept override;
	focus_result on_get_focus(sys::state& state) noexcept override;
	void on_lose_focus(sys::state& state) noexcept override;
	void set_cursor_visibility(sys::state& state, bool visible) noexcept override;
	void edit_move_cursor_to_screen_point(sys::state& state, int32_t x, int32_t y, bool extend_selection) noexcept override;
	sys::text_mouse_test_result detailed_text_mouse_test(sys::state& state, int32_t x, int32_t y) noexcept override;
	void set_temporary_selection(sys::state& state, int32_t start, int32_t end) noexcept override;
	void register_composition_result(sys::state& state) noexcept override;
	std::u16string_view text_content() noexcept override { return cached_text; }
	std::pair<int32_t, int32_t> text_selection() noexcept override;
	std::pair<int32_t, int32_t> temporary_text_range() noexcept override;
	void set_text_selection(sys::state& state, int32_t cursor, int32_t anchor) noexcept override;
	void insert_text(sys::state& state, int32_t position_start, int32_t position_end, std::u16string_view content, insertion_source source) noexcept override;
	bool position_is_ltr(int32_t position) noexcept override;
	ui::urect text_bounds(sys::state& state, int32_t position_start, int32_t position_end) noexcept override;
	void on_hover(sys::state& state) noexcept override;
	void on_hover_end(sys::state& state) noexcept override;

	~edit_box_element_base() override;
};

class tool_tip : public element_base {
public:
	text::layout internal_layout;
	tool_tip() { }
	void render(sys::state& state, int32_t x, int32_t y) noexcept override;
};


void render_text_chunk(
	sys::state& state,
	text::text_chunk t,
	float x,
	float baseline_y,
	uint16_t font_id,
	ogl::color3f text_color,
	ogl::color_modification cmod
);

} // namespace ui
