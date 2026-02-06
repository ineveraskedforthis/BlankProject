#include "locale.hpp"
#include "hb.h"
#include "parsers.hpp"

namespace locale {
locale_parser parse_locale_parser(parsers::token_generator& gen, parsers::error_handler& err, sys::state& context) {
	locale_parser cobj;
	for(parsers::token_and_type cur = gen.get(); cur.type != parsers::token_type::unknown && cur.type != parsers::token_type::close_brace; cur = gen.get()) {
		if(cur.type == parsers::token_type::open_brace) {
			err.unhandled_free_group(cur); gen.discard_group();
			continue;
		}
		auto peek_result = gen.next();
		if(peek_result.type == parsers::token_type::special_identifier) {
			auto peek2_result = gen.next_next();
			if(peek2_result.type == parsers::token_type::open_brace) {
				gen.get(); gen.get();
				switch(int32_t(cur.content.length())) {
				default:
					err.unhandled_group_key(cur); gen.discard_group();
					break;
				}
			} else {
				auto const assoc_token = gen.get();
				auto const assoc_type = parse_association_type(assoc_token.content, assoc_token.line, err);
				auto const rh_token = gen.get();
				switch(int32_t(cur.content.length())) {
				case 3:
					// rtl
					if((true && (*(uint16_t const*)(&cur.content[0]) | 0x2020) == 0x7472 && (cur.content[2] | 0x20) == 0x6C)) {
						cobj.rtl = parse_bool(rh_token.content, rh_token.line, err);
					} else {
						err.unhandled_association_key(cur);
					}
					break;
				case 6:
					// script
					if((true && (*(uint32_t const*)(&cur.content[0]) | uint32_t(0x20202020)) == uint32_t(0x69726373) && (*(uint16_t const*)(&cur.content[4]) | 0x2020) == 0x7470)) {
						cobj.script = parse_text(rh_token.content, rh_token.line, err);
					} else {
						err.unhandled_association_key(cur);
					}
					break;
				case 8:
					switch(0x20 | int32_t(cur.content[0])) {
					case 0x66:
						// fallback
						if((true && (*(uint32_t const*)(&cur.content[1]) | uint32_t(0x20202020)) == uint32_t(0x626C6C61) && (*(uint16_t const*)(&cur.content[5]) | 0x2020) == 0x6361 && (cur.content[7] | 0x20) == 0x6B)) {
							cobj.fallback = parse_text(rh_token.content, rh_token.line, err);
						} else {
							err.unhandled_association_key(cur);
						}
						break;
					default:
						err.unhandled_association_key(cur);
						break;
					}
					break;
				case 9:
					// body_font
					if((true && (*(uint64_t const*)(&cur.content[0]) | uint64_t(0x2020202020202020)) == uint64_t(0x6E6F667F79646F62) && (cur.content[8] | 0x20) == 0x74)) {
						cobj.body_font = parse_text(rh_token.content, rh_token.line, err);
					} else {
						err.unhandled_association_key(cur);
					}
					break;
				case 11:
					switch(0x20 | int32_t(cur.content[0])) {
					case 0x68:
						// header_font
						if((true && (*(uint64_t const*)(&cur.content[1]) | uint64_t(0x2020202020202020)) == uint64_t(0x6F667F7265646165) && (*(uint16_t const*)(&cur.content[9]) | 0x2020) == 0x746E)) {
							cobj.header_font = parse_text(rh_token.content, rh_token.line, err);
						} else {
							err.unhandled_association_key(cur);
						}
						break;
					default:
						err.unhandled_association_key(cur);
						break;
					}
					break;
				case 12:
					switch(0x20 | int32_t(cur.content[0])) {
					case 0x62:
						// body_feature
						if((true && (*(uint64_t const*)(&cur.content[1]) | uint64_t(0x2020202020202020)) == uint64_t(0x746165667F79646F) && (*(uint16_t const*)(&cur.content[9]) | 0x2020) == 0x7275 && (cur.content[11] | 0x20) == 0x65)) {
							cobj.body_feature(assoc_type, parse_text(rh_token.content, rh_token.line, err), err, cur.line, context);
						} else {
							err.unhandled_association_key(cur);
						}
						break;
					case 0x64:
						// display_name
						if((true && (*(uint64_t const*)(&cur.content[1]) | uint64_t(0x2020202020202020)) == uint64_t(0x6E7F79616C707369) && (*(uint16_t const*)(&cur.content[9]) | 0x2020) == 0x6D61 && (cur.content[11] | 0x20) == 0x65)) {
							cobj.display_name = parse_text(rh_token.content, rh_token.line, err);
						} else {
							err.unhandled_association_key(cur);
						}
						break;
					default:
						err.unhandled_association_key(cur);
						break;
					}
					break;
				case 14:
					// header_feature
					if((true && (*(uint64_t const*)(&cur.content[0]) | uint64_t(0x2020202020202020)) == uint64_t(0x667F726564616568) && (*(uint32_t const*)(&cur.content[8]) | uint32_t(0x20202020)) == uint32_t(0x75746165) && (*(uint16_t const*)(&cur.content[12]) | 0x2020) == 0x6572)) {
						cobj.header_feature(assoc_type, parse_text(rh_token.content, rh_token.line, err), err, cur.line, context);
					} else {
						err.unhandled_association_key(cur);
					}
					break;
				default:
					err.unhandled_association_key(cur);
					break;
				}
			}
		} else {
			err.unhandled_free_value(cur);
		}
	}
	cobj.finish(context);
	return cobj;
}

void add_locale(std::string_view locale_name, char const* data_start, char const* data_end) {
	parsers::token_generator gen(data_start, data_end);
	parsers::error_handler err("");

	locale_parser new_locale = parse_locale_parser(gen, err, state);
	hb_language_t lang = nullptr;

	auto new_locale_id = state.world.create_locale();
	auto new_locale_obj = fatten(state.world, new_locale_id);
	new_locale_obj.set_hb_script(hb_script_from_string(new_locale.script.c_str(), int(new_locale.script.length())));
	new_locale_obj.set_native_rtl(new_locale.rtl);

	{
		auto f = new_locale_obj.get_body_font();
		f.resize(uint32_t(new_locale.body_font.length()));
		f.load_range((uint8_t const*)new_locale.body_font.c_str(), (uint8_t const*)new_locale.body_font.c_str() + new_locale.body_font.length());
	}
	{
		auto f = new_locale_obj.get_header_font();
		f.resize(uint32_t(new_locale.header_font.length()));
		f.load_range((uint8_t const*)new_locale.header_font.c_str(), (uint8_t const*)new_locale.header_font.c_str() + new_locale.header_font.length());
	}

	{
		auto f = new_locale_obj.get_body_font_features();
		f.resize(uint32_t(new_locale.body_features.size()));
		f.load_range(new_locale.body_features.data(), new_locale.body_features.data() + new_locale.body_features.size());
	}
	{
		auto f = new_locale_obj.get_header_font_features();
		f.resize(uint32_t(new_locale.header_features.size()));
		f.load_range(new_locale.header_features.data(), new_locale.header_features.data() + new_locale.header_features.size());
	}

	{
		auto f = new_locale_obj.get_locale_name();
		f.resize(uint32_t(locale_name.length()));
		f.load_range((uint8_t const*)locale_name.data(), (uint8_t const*)locale_name.data() + locale_name.length());
	}
	{
		auto f = new_locale_obj.get_fallback();
		f.resize(uint32_t(new_locale.fallback.length()));
		f.load_range((uint8_t const*)new_locale.fallback.data(), (uint8_t const*)new_locale.fallback.data() + new_locale.fallback.length());
	}
	{
		auto f = new_locale_obj.get_display_name();
		f.resize(uint32_t(new_locale.display_name.length()));
		f.load_range((uint8_t const*)new_locale.display_name.data(), (uint8_t const*)new_locale.display_name.data() + new_locale.display_name.length());
	}
}

}