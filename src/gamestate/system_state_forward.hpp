#include <cstdint>

namespace sys {
struct state;
}
namespace alice_ui {
bool state_is_rtl(sys::state& state);
}
namespace text {
enum class text_color : uint8_t;
}
namespace ogl {
struct color3f;
}
