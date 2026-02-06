#pragma once
#include <numbers>
#include "parsers.hpp"

#include <string>
#include <string_view>

#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif
#include "GL/glew.h"

#include "container_types.hpp"
#include "container_types_ui.hpp"
#include "texture.hpp"
#include "fonts.hpp"
#include "constants_ui.hpp"
#include "glm/glm.hpp"

namespace text {
class font;
struct stored_glyphs;
}

namespace ogl {

enum class color_modification {
	none, disabled, interactable, interactable_disabled
};

inline color3f unpack_color(uint32_t v) {
	return color3f{ sys::red_from_int(v), sys::green_from_int(v), sys::blue_from_int(v) };
}

struct image {
	uint8_t* data = nullptr;
	int32_t size_x = 0;
	int32_t size_y = 0;
	int32_t channels = 0;

	image() { }

	image(uint8_t* data, int32_t size_x, int32_t size_y, int32_t channels) {
		this->data = data;
		this->size_x = size_x;
		this->size_y = size_y;
		this->channels = channels;
	}
	image(image const& other) = delete;

	image(image&& other) noexcept {
		data = other.data;
		size_x = other.size_x;
		size_y = other.size_y;
		channels = other.channels;
		other.data = nullptr;
	}

	image& operator=(image&& other) noexcept {
		data = other.data;
		size_x = other.size_x;
		size_y = other.size_y;
		channels = other.channels;
		other.data = nullptr;
		return *this;
	}
	image& operator=(image const& other) = delete;

	~image() {
		if(data)
			free(data);
	}
};

#ifndef NDEBUG
inline void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
		GLsizei, // length
		GLchar const* message, void const*) {

	std::string source_str;
	switch(source) {
	case GL_DEBUG_SOURCE_API:
		source_str = "OpenGL API call";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		source_str = "Window system API";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		source_str = "Shading language compiler";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		source_str = "Application associated with OpenGL";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		source_str = "User generated";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		source_str = "Unknown source";
		break;
	default:
		break;
	}
	std::string error_type;
	switch(type) {
	case GL_DEBUG_TYPE_ERROR:
		error_type = "General error";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		error_type = "Deprecated behavior";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		error_type = "Undefined behavior";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		error_type = "Portability issue";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		error_type = "Performance issue";
		break;
	case GL_DEBUG_TYPE_MARKER:
		error_type = "Command stream annotation";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		error_type = "Error group push";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		error_type = "Error group pop";
		break;
	case GL_DEBUG_TYPE_OTHER:
		error_type = "Unknown error type";
		break;
	default:
		break;
	}
	std::string severity_str;
	switch(severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		severity_str = "High";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		severity_str = "Medium";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		severity_str = "Low";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		severity_str = "Notification";
		return; // don't print notifications
	default:
		break;
	}
	std::string full_message("OpenGL error ");
	full_message += std::to_string(id);
	full_message += " ";
	full_message += "; Source: ";
	full_message += source_str;
	full_message += " ; Type: ";
	full_message += error_type;
	full_message += "; Severity: ";
	full_message += severity_str;
	full_message += "; ";
	full_message += message;
	full_message += "\n";

#ifdef _WIN64
	OutputDebugStringA(full_message.c_str());
#else
	printf("%s", full_message.c_str());
#endif
}
#endif

struct data {
	tagged_vector<texture, dcon::texture_id> asset_textures;
	ankerl::unordered_dense::map<std::string, dcon::texture_id> late_loaded_map;

	void* context = nullptr;
	bool legacy_mode = false;
	GLuint ui_shader_program = 0;

	GLuint province_map_framebuffer;
	GLuint province_map_rendertexture;
	GLuint province_map_depthbuffer;

	GLuint ui_shader_d_rect_uniform = 0;
	GLuint ui_shader_subroutines_index_uniform = 0;
	GLuint ui_shader_inner_color_uniform = 0;
	GLuint ui_shader_subrect_uniform = 0;
	GLuint ui_shader_border_size_uniform = 0;
	GLuint ui_shader_texture_sampler_uniform = 0;
	GLuint ui_shader_secondary_texture_sampler_uniform = 0;
	GLuint ui_shader_screen_width_uniform = 0;
	GLuint ui_shader_screen_height_uniform = 0;
	GLuint ui_shader_gamma_uniform = 0;

	GLuint global_square_vao = 0;
	GLuint global_square_buffer = 0;
	GLuint global_square_right_buffer = 0;
	GLuint global_square_left_buffer = 0;
	GLuint global_square_flipped_buffer = 0;
	GLuint global_square_right_flipped_buffer = 0;
	GLuint global_square_left_flipped_buffer = 0;

	GLuint global_rtl_square_buffer = 0;
	GLuint global_rtl_square_right_buffer = 0;
	GLuint global_rtl_square_left_buffer = 0;
	GLuint global_rtl_square_flipped_buffer = 0;
	GLuint global_rtl_square_right_flipped_buffer = 0;
	GLuint global_rtl_square_left_flipped_buffer = 0;

	GLuint money_icon_tex = 0;
	GLuint cross_icon_tex = 0;
	GLuint cross_desaturated_icon_tex = 0;
	GLuint color_blind_cross_icon_tex = 0;
	GLuint checkmark_icon_tex = 0;
	GLuint checkmark_desaturated_icon_tex = 0;
	GLuint navy_icon_tex = 0;
	GLuint army_icon_tex = 0;

	GLuint msaa_framebuffer = 0;
	GLuint msaa_texcolorbuffer = 0;
	GLuint msaa_rbo = 0;
	GLuint msaa_interbuffer = 0;
	GLuint msaa_texture = 0;
	GLuint msaa_vao = 0;
	GLuint msaa_vbo = 0;
	GLuint msaa_shader_program = 0;
	GLuint msaa_uniform_screen_size = 0;
	GLuint msaa_uniform_gaussian_blur = 0;
	bool msaa_enabled = false;
};

void notify_user_of_fatal_opengl_error(std::string message);
std::string_view opengl_get_error_name(GLenum t);

void create_opengl_context(ogl::data& state); // you shouldn't call this directly; only initialize_opengl should call it
void initialize_opengl(ogl::data& state);
void shutdown_opengl(ogl::data& state);

bool display_tag_is_valid(ogl::data& state, char tag[3]);

GLint compile_shader(std::string_view source, GLenum type);
GLuint create_program(std::string_view vertex_shader, std::string_view fragment_shader);
GLuint create_program(std::string_view vertex_shader, std::string_view tes_control_shader, std::string_view tes_eval_shader, std::string_view fragment_shader, bool debug_geom_shader);
void load_shaders(ogl::data& state, simple_fs::file_system& fs);
void load_global_squares(ogl::data& state);

class bezier_path {
public:

	struct bezier_vertex {
		bezier_vertex() {
		};
		bezier_vertex(glm::vec2 base_point_0, glm::vec2 base_point_1, glm::vec2 control_point_0, glm::vec2 control_point_1, float length_offset)
			: base_point_0(base_point_0), base_point_1(base_point_1), control_point_0(control_point_0), control_point_1(control_point_1), length_offset{ length_offset } {
		};
		glm::vec2 base_point_0;
		glm::vec2 base_point_1;
		glm::vec2 control_point_0;
		glm::vec2 control_point_1;
		float length_offset;
	};
	struct extra_data_s {
		float length = 0.0f;
		float width = 0.0f;
	};

	static constexpr int32_t extra_data_resolution = 8;

	std::vector<bezier_vertex> path_data;
	std::vector<extra_data_s> extra_data;

	GLuint data_vbo = 0;
	GLuint data_vao = 0;
	GLuint dat_texture = 0;
	GLuint dat_buffer = 0;

	bezier_path() = default;
	bezier_path(bezier_path&& other) : path_data(std::move(other.path_data)) {
		data_vbo = other.data_vbo;
		other.data_vbo = 0;
		data_vao = other.data_vao;
		other.data_vao = 0;
		dat_texture = other.dat_texture;
		other.dat_texture = 0;
		dat_buffer = other.dat_buffer;
		other.dat_buffer = 0;
	}
	bezier_path& operator=(bezier_path&& other) {
		path_data = std::move(other.path_data);
		data_vao = other.data_vao;
		other.data_vao = 0;
		data_vao = other.data_vao;
		other.data_vao = 0;
		dat_texture = other.dat_texture;
		other.dat_texture = 0;
		dat_buffer = other.dat_buffer;
		other.dat_buffer = 0;
		return *this;
	}
	~bezier_path();
	void update_vbo();
	void render();
};

class lines {
private:
	float* buffer = nullptr;
	GLuint buffer_handle = 0;
	bool pending_data_update = true;

public:
	uint32_t count = 0;

	lines(uint32_t c) : count(c) {
		buffer = new float[count * 4];
		set_default_y();
	}
	lines(lines&& o) noexcept
			: buffer(o.buffer), buffer_handle(o.buffer_handle), pending_data_update(o.pending_data_update), count(o.count) {
		o.buffer = nullptr;
	}
	lines& operator=(lines&& o) noexcept {
		count = o.count;
		buffer = o.buffer;
		buffer_handle = o.buffer_handle;
		pending_data_update = o.pending_data_update;

		o.buffer = nullptr;
		return *this;
	}
	~lines() {
		delete[] buffer;
	}
	void set_default_y();
	void set_y(float* v);
	void bind_buffer();
};

class generic_ui_mesh_triangle_strip {
private:
	float* buffer = nullptr;
	GLuint buffer_handle = 0;
	bool pending_data_update = true;

public:
	uint32_t count = 0;

	generic_ui_mesh_triangle_strip(uint32_t c) : count(c) {
		buffer = new float[count * 4];
		set_default();
	}
	generic_ui_mesh_triangle_strip(generic_ui_mesh_triangle_strip&& o) noexcept
		: buffer(o.buffer), buffer_handle(o.buffer_handle), pending_data_update(o.pending_data_update), count(o.count) {
		o.buffer = nullptr;
	}
	generic_ui_mesh_triangle_strip& operator=(generic_ui_mesh_triangle_strip&& o) noexcept {
		count = o.count;
		buffer = o.buffer;
		buffer_handle = o.buffer_handle;
		pending_data_update = o.pending_data_update;

		o.buffer = nullptr;
		return *this;
	}
	~generic_ui_mesh_triangle_strip() {
		delete[] buffer;
	}
	void set_default();
	void set_coords(float* v);
	void bind_buffer();
};

std::string_view framebuffer_error(GLenum e);

void render_colored_rect(ogl::data const& state, float x, float y, float width, float height, float red, float green, float blue, ui::rotation r, bool flipped, bool rtl);
void render_alpha_colored_rect(ogl::data const& state, float x, float y, float width, float height, float red, float green, float blue, float alpha);
void render_simple_rect(ogl::data const& state, float x, float y, float width, float height, ui::rotation r, bool flipped, bool rtl);
void render_textured_rect(ogl::data const& state, color_modification enabled, float x, float y, float width, float height, GLuint texture_handle, ui::rotation r, bool flipped, bool rtl);
void render_textured_rect_direct(ogl::data const& state, float x, float y, float width, float height, uint32_t handle);
void render_linegraph(ogl::data const& state, color_modification enabled, float x, float y, float width, float height, lines& l);
void render_linegraph(ogl::data const& state, color_modification enabled, float x, float y, float width, float height, float r, float g, float b, lines& l);
void render_linegraph(ogl::data const& state, color_modification enabled, float x, float y, float width, float height, float r, float g, float b, float a, lines& l);
void render_barchart(ogl::data const& state, color_modification enabled, float x, float y, float width, float height, data_texture& t, ui::rotation r, bool flipped, bool rtl);
void render_ui_mesh( ogl::data const& state, color_modification enabled, float x, float y, float width, float height, generic_ui_mesh_triangle_strip& mesh, data_texture& t);
void render_piechart(ogl::data const& state, color_modification enabled, float x, float y, float size, data_texture& t);
void render_stripchart(ogl::data const& state, color_modification enabled, float x, float y, float sizex, float sizey, data_texture& t);
void render_bordered_rect(ogl::data const& state, color_modification enabled, float border_size, float x, float y, float width, float height, GLuint texture_handle, ui::rotation r, bool flipped, bool rtl);
void render_rect_with_repeated_border(ogl::data const& state, color_modification enabled, float grid_size, float x, float y, float width, float height, GLuint texture_handle, ui::rotation r, bool flipped, bool rtl);
void render_rect_with_repeated_corner(ogl::data const& state, color_modification enabled, float grid_size, float x, float y, float width, float height, GLuint texture_handle, ui::rotation r, bool flipped, bool rtl);
void render_masked_rect(ogl::data const& state, color_modification enabled, float x, float y, float width, float height, GLuint texture_handle, GLuint mask_texture_handle, ui::rotation r, bool flipped, bool rtl);
void render_progress_bar(ogl::data const& state, color_modification enabled, float progress, float x, float y, float width, float height, GLuint left_texture_handle, GLuint right_texture_handle, ui::rotation r, bool flipped, bool rtl);
void render_tinted_textured_rect(ogl::data const& state, float x, float y, float width, float height, float r, float g, float b, GLuint texture_handle, ui::rotation rot, bool flipped, bool rtl);
void render_subsprite(ogl::data const& state, color_modification enabled, int frame, int total_frames, float x, float y, float width, float height, GLuint texture_handle, ui::rotation r, bool flipped, bool rtl);
void render_rect_slice(ogl::data const& state, float x, float y, float width, float height, GLuint texture_handle, float start_slice, float end_slice);
void render_tinted_rect(ogl::data const& state, float x, float y, float width, float height, float r, float g, float b, ui::rotation rot, bool flipped, bool rtl);
void render_tinted_subsprite(ogl::data const& state, int frame, int total_frames, float x, float y, float width, float height, float r, float g, float b, GLuint texture_handle, ui::rotation rot, bool flipped, bool rtl);
void render_new_text(
	data& state,
	text::font_manager& font_collection,
	text::font& f,
	text::stored_glyphs const& txt,
	color_modification enabled,
	float x,
	float y,
	float size,
	color3f const& c,
	float ui_scale
);
void render_text_icon(
	ogl::data& state,
	text::font_manager& font_collection,
	text::embedded_icon ico,
	float x,
	float baseline_y,
	float font_size,
	text::font& f,
	ogl::color_modification cmod,
	float ui_scale
);

void deinitialize_framebuffer_for_province_indices(ogl::data& state);
void initialize_framebuffer_for_province_indices(ogl::data& state, int32_t size_x, int32_t size_y);

bool msaa_enabled(ogl::data const& state);
void initialize_msaa(ogl::data& state, int32_t x, int32_t y);
void deinitialize_msaa(ogl::data& state);

image load_stb_image(simple_fs::file& file);
GLuint make_gl_texture(uint8_t* data, uint32_t size_x, uint32_t size_y, uint32_t channels);
GLuint make_gl_texture(simple_fs::directory const& dir, native_string_view file_name);
void set_gltex_parameters(GLuint texture_handle, GLuint texture_type, GLuint filter, GLuint wrap);
void set_gltex_parameters(GLuint texture_handle, GLuint texture_type, GLuint filter, GLuint wrap_a, GLuint wrap_b);
GLuint load_texture_array_from_file(simple_fs::file& file, int32_t tiles_x, int32_t tiles_y);
void load_special_icons(ogl::data& state, simple_fs::file_system& fs);

void render_subrect(ogl::data const& state, float target_x, float target_y, float target_width, float target_height, float source_x, float source_y, float source_width, float source_height, GLuint texture_handle);


} // namespace ogl
