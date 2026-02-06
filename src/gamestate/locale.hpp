#include <vector>

#include "hb.h"
#include "parsers.hpp"


namespace dcon {
        struct data_container;
}

namespace locale {
struct locale_parser {
        bool rtl = false;
        std::string display_name;
        std::string script = "Latn";
        std::string body_font;
        std::string header_font;
        std::string fallback;
        std::vector<uint32_t> body_features;
        std::vector<uint32_t> header_features;

        void body_feature(parsers::association_type, std::string_view value, parsers::error_handler& err, int32_t line) {
                body_features.push_back(hb_tag_from_string(value.data(), int(value.length())));
        }
        void header_feature(parsers::association_type, std::string_view value, parsers::error_handler& err, int32_t line) {
                header_features.push_back(hb_tag_from_string(value.data(), int(value.length())));
        }

        void finish() {
        }
};

locale_parser parse_locale_parser(parsers::token_generator& gen, parsers::error_handler& err);
void add_locale(dcon::data_container& data, std::string_view locale_name, char const* data_start, char const* data_end);

}