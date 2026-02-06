#pragma once

#include "freetype/freetype.h"
#include "freetype/ftglyph.h"
#include "unordered_dense.h"
#include "hb.h"
#include <common_types.hpp>
#include <span>
#include "data_ids.hpp"
#include "simple_fs.hpp"

namespace dcon {
struct data_container;
}

namespace text {

using font_id = uint16_t;


inline constexpr uint32_t max_texture_layers = 256;
inline constexpr int magnification_factor = 4;
inline constexpr int dr_size = 64 * magnification_factor;

enum class font_selection {
	body_font,
	header_font
};

// int32_t size_from_font_id(uint16_t id);
// bool is_black_from_font_id(uint16_t id);
// font_selection font_index_from_font_id(uint16_t id);

struct glyph_sub_offset {
	uint16_t x = 0;
	uint16_t y = 0;
	uint16_t width = 0;
	uint16_t height = 0;
	uint16_t tx_sheet = 0;
	int16_t bitmap_left = 0;
	int16_t bitmap_top = 0;
};

class font_manager;

enum class font_feature {
	none, small_caps
};

class font;

inline bool requires_surrogate_pair(uint32_t codepoint) {
	return codepoint >= 0x10000;
}

struct surrogate_pair {
	uint16_t high = 0; // aka leading
	uint16_t low = 0; // aka trailing
};

inline surrogate_pair make_surrogate_pair(uint32_t val) noexcept {
	uint32_t v = val - 0x10000;
	uint32_t h = ((v >> 10) & 0x03FF) | 0xD800;
	uint32_t l = (v & 0x03FF) | 0xDC00;
	return surrogate_pair{ uint16_t(h), uint16_t(l) };
}

struct ex_grapheme_cluster_info {
	uint16_t source_offset = 0; // index of first codepoint in cluster within source text
	int16_t x_offset = 0; // ui x position of rendered grapheme cluster
	int16_t width = 0;  // ui size of rendered grapheme cluster
	int16_t visual_left = -1; // index of grapheme cluster to the left, or -1 if none
	int16_t visual_right = -1; // index of grapheme cluster to the right, or -1 if none

	uint8_t flags = 0;
	uint8_t line = 0; // which line in the layout, starting at 0
	uint8_t unit_length = 0; // how many utf16 codepoints the cluster consists of

	constexpr static uint8_t f_is_word_start = 0x01;
	constexpr static uint8_t f_is_word_end = 0x02;
	constexpr static uint8_t f_has_rtl_directionality = 0x10;

	inline bool has_rtl_directionality() {
		return (flags & f_has_rtl_directionality) != 0;
	}
	inline bool is_word_start() {
		return (flags & f_is_word_start) != 0;
	}
	inline bool is_word_end() {
		return (flags & f_is_word_end) != 0;
	}
};

struct stored_glyph {
	uint32_t codepoint = 0;
	uint32_t cluster = 0;
	hb_position_t  x_advance = 0;
	hb_position_t  y_advance = 0;
	hb_position_t  x_offset = 0;
	hb_position_t  y_offset = 0;

	stored_glyph() noexcept = default;
	stored_glyph(hb_glyph_info_t const& gi, hb_glyph_position_t const& gp) {
		codepoint = gi.codepoint;
		cluster = gi.cluster;
		x_advance = gp.x_advance;
		y_advance = gp.y_advance;
		x_offset = gp.x_offset;
		y_offset = gp.y_offset;
	}
};

struct layout_details {
	std::vector<ex_grapheme_cluster_info> grapheme_placement;
	uint8_t total_lines = 0;
};

struct stored_glyphs {
	std::vector<stored_glyph> glyph_info;

	struct no_bidi { };

	stored_glyphs() = default;
	stored_glyphs(stored_glyphs const& other) noexcept = default;
	stored_glyphs(stored_glyphs&& other) noexcept = default;
	stored_glyphs(stored_glyphs& other, uint32_t offset, uint32_t count);
	stored_glyphs(
		font_manager& font_collection,
		int32_t size,
		std::span<uint16_t> s,
		layout_details* d,
		uint32_t details_offset,
		font_id f,
		dcon::dcon_vv_fat_id<uint32_t> features,
		hb_script_t hb_script,
		hb_language_t language,
		bool rtl,
		float ui_scale
	);
	stored_glyphs(
		font_manager& font_collection,
		int32_t size,
		std::span<uint16_t> source,
		no_bidi,
		font_id f,
		dcon::dcon_vv_fat_id<uint32_t> features,
		hb_script_t hb_script,
		hb_language_t language,
		bool rtl,
		float ui_scale
	);

	//void set_text(sys::state& state, font_selection type, std::string const& s);
	void clear() {
		glyph_info.clear();
	}
};

class font_at_size {
private:
	float internal_line_height = 0.0f;
	float internal_ascender = 0.0f;
	float internal_descender = 0.0f;
	float internal_top_adj = 0.0f;

	uint32_t internal_tx_line_height = 0;
	uint32_t internal_tx_line_xpos = 1024;
	uint32_t internal_tx_line_ypos = 1024;
	int32_t px_size = 0;
	ankerl::unordered_dense::map<uint32_t, glyph_sub_offset> glyph_positions{};
public:
	FT_Face font_face = nullptr;
	hb_font_t* hb_font_face = nullptr;
	hb_buffer_t* hb_buf = nullptr;


	std::vector<uint32_t> textures;

	void make_glyph(uint16_t glyph_in, int32_t subpixel);
	glyph_sub_offset& get_glyph(uint16_t glyph_in, int32_t subpixel);
	void reset();
	void create(FT_Library lib, FT_Byte* file_data, size_t file_size, int32_t real_size);
	void remake_cache(
		font_manager& font_collection,
		stored_glyphs& txt,
		std::span<uint16_t> source,
		layout_details* d,
		uint32_t details_offset,
		font_id f,
		dcon::dcon_vv_fat_id<uint32_t> features,
		hb_script_t hb_script,
		hb_language_t language,
		bool rtl,
		float ui_scale
	);
	void remake_bidiless_cache(
		font_manager& font_collection,
		stored_glyphs& txt,
		std::span<uint16_t> source,
		font_id f,
		dcon::dcon_vv_fat_id<uint32_t> features,
		hb_script_t hb_script,
		hb_language_t language,
		bool rtl,
		float ui_scale
	);
	float line_height(float ui_scale) const;
	float ascender(float ui_scale) const;
	float descender(float ui_scale) const;
	float top_adjustment(float ui_scale) const;
	float text_extent(stored_glyphs const& txt, uint32_t starting_offset, uint32_t count, float ui_scale);
	float text_extent(char const* codepoints, uint32_t count, float ui_scale);

	font_at_size() = default;
	font_at_size(font_at_size&& o) noexcept : glyph_positions(std::move(o.glyph_positions)), textures(o.textures) {
		font_face = o.font_face;
		o.font_face = nullptr;
		hb_font_face = o.hb_font_face;
		o.hb_font_face = nullptr;
		hb_buf = o.hb_buf;
		o.hb_buf = nullptr;
		internal_line_height = o.internal_line_height;
		internal_ascender = o.internal_ascender;
		internal_descender = o.internal_descender;
		internal_top_adj = o.internal_top_adj;
		internal_tx_line_height = o.internal_tx_line_height;
		internal_tx_line_xpos = o.internal_tx_line_xpos;
		internal_tx_line_ypos = o.internal_tx_line_ypos;
	}
	font_at_size& operator=(font_at_size&& o) noexcept {
		glyph_positions = std::move(o.glyph_positions);
		textures = std::move(o.textures);
		font_face = o.font_face;
		o.font_face = nullptr;
		hb_font_face = o.hb_font_face;
		o.hb_font_face = nullptr;
		hb_buf = o.hb_buf;
		o.hb_buf = nullptr;
		internal_line_height = o.internal_line_height;
		internal_ascender = o.internal_ascender;
		internal_descender = o.internal_descender;
		internal_top_adj = o.internal_top_adj;
		internal_tx_line_height = o.internal_tx_line_height;
		internal_tx_line_xpos = o.internal_tx_line_xpos;
		internal_tx_line_ypos = o.internal_tx_line_ypos;
		return *this;
	}
};

class font {
private:
	font(font const&) = delete;
	font& operator=(font const&) = delete;
public:
	font() = default;

	ankerl::unordered_dense::map<int32_t, font_at_size> sized_fonts;
	std::string file_name;

	std::unique_ptr<FT_Byte[]> file_data;
	size_t file_size = 0;

	~font();

	bool can_display(char32_t ch_in) const;
	font_at_size& retrieve_instance(text::font_manager& font_collection, int32_t base_size, float ui_scale);
	font_at_size& retrieve_stateless_instance(FT_Library lib, int32_t base_size);
	void reset_instances();

	friend class font_manager;

	font(font&& o) noexcept : file_name(std::move(o.file_name)),  file_data(std::move(o.file_data)) {
		file_size = o.file_size;
	}
	font& operator=(font&& o) noexcept {
		file_name = std::move(o.file_name);
		file_data = std::move(o.file_data);
		file_size = o.file_size;
		o.file_size = 0;
		return *this;
	}
};


class font_manager {
public:
	font_manager();
	~font_manager();

	ankerl::unordered_dense::map<uint16_t, dcon::text_key> font_names;
	FT_Library ft_library;
private:
	std::vector<font> font_array;
	font_manager(font_manager const&) = delete;
	font_manager& operator=(font_manager const&) = delete;
public:
	std::vector<uint8_t> compiled_ubrk_rules;
	std::vector<uint8_t> compiled_char_ubrk_rules;
	std::vector<uint8_t> compiled_word_ubrk_rules;
	bool map_font_is_black = false;

	// dcon::locale_id current_locale;
	void resolve_locale(dcon::data_container& data, simple_fs::file_system& fs, dcon::locale_id l);

	void reset_fonts();
	font& get_font(font_id f);
	void load_font(font& fnt, char const* file_data, uint32_t file_size);
	float line_height(
		font_id f,
		uint16_t size,
		float ui_scale
	);
	float text_extent(
		stored_glyphs const& txt,
		uint32_t starting_offset,
		uint32_t count,
		font_id f,
		int16_t font_size,
		float ui_scale
	);

};

font_id make_font_id(bool as_header, float target_line_size);

} // namespace text
