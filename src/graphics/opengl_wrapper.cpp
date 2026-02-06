#include "opengl_wrapper.hpp"
#include "simple_fs.hpp"
#include "fonts.hpp"

#include "constants.hpp"
#include "window.hpp"

#undef STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace ogl {

GLuint map_color_modification_to_index(color_modification e) {
	switch(e) {
	case color_modification::disabled:
		return parameters::disabled;
	case color_modification::interactable:
		return parameters::interactable;
	case color_modification::interactable_disabled:
		return parameters::interactable_disabled;
	default:
	case color_modification::none:
		return parameters::enabled;
	}
}

std::string_view opengl_get_error_name(GLenum t) {
	switch(t) {
		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
		case GL_STACK_UNDERFLOW:
			return "GL_STACK_UNDERFLOW";
		case GL_STACK_OVERFLOW:
			return "GL_STACK_OVERFLOW";
		case GL_NO_ERROR:
			return "GL_NO_ERROR";
		default:
			return "Unknown";
	}
}

void notify_user_of_fatal_opengl_error(std::string message) {
	std::string full_message = message;
	full_message += "\n";
	full_message += opengl_get_error_name(glGetError());
	window::emit_error_message("OpenGL error:" + full_message, true);
}


GLint compile_shader(std::string_view source, GLenum type) {
	GLuint return_value = glCreateShader(type);

	if(return_value == 0) {
		notify_user_of_fatal_opengl_error("shader creation failed");
	}

	std::string s_source(source);
	GLchar const* texts[] = {
		"#version 330 core\r\n",
		"#extension GL_ARB_explicit_uniform_location : enable\r\n",
		"#extension GL_ARB_explicit_attrib_location : enable\r\n",
		"#extension GL_ARB_shader_subroutine : enable\r\n",
		"#extension GL_ARB_vertex_array_object : enable\r\n"
		"#define M_PI 3.1415926535897932384626433832795\r\n",
		"#define PI 3.1415926535897932384626433832795\r\n",
		s_source.c_str()
	};
	glShaderSource(return_value, 7, texts, nullptr);
	glCompileShader(return_value);

	GLint result;
	glGetShaderiv(return_value, GL_COMPILE_STATUS, &result);
	if(result == GL_FALSE) {
		GLint log_length = 0;
		glGetShaderiv(return_value, GL_INFO_LOG_LENGTH, &log_length);

		auto log = std::unique_ptr<char[]>(new char[static_cast<size_t>(log_length)]);
		GLsizei written = 0;
		glGetShaderInfoLog(return_value, log_length, &written, log.get());
		notify_user_of_fatal_opengl_error(std::string("Shader failed to compile:\n") + log.get());
	}
	return return_value;
}

GLuint create_program(std::string_view vertex_shader, std::string_view fragment_shader) {
	GLuint return_value = glCreateProgram();
	if(return_value == 0) {
		notify_user_of_fatal_opengl_error("program creation failed");
	}

	auto v_shader = compile_shader(vertex_shader, GL_VERTEX_SHADER);
	auto f_shader = compile_shader(fragment_shader, GL_FRAGMENT_SHADER);

	glAttachShader(return_value, v_shader);
	glAttachShader(return_value, f_shader);
	glLinkProgram(return_value);

	GLint result;
	glGetProgramiv(return_value, GL_LINK_STATUS, &result);
	if(result == GL_FALSE) {
		GLint logLen;
		glGetProgramiv(return_value, GL_INFO_LOG_LENGTH, &logLen);

		char* log = new char[static_cast<size_t>(logLen)];
		GLsizei written;
		glGetProgramInfoLog(return_value, logLen, &written, log);
		notify_user_of_fatal_opengl_error(std::string("Program failed to link:\n") + log);
	}

	glDeleteShader(v_shader);
	glDeleteShader(f_shader);

	return return_value;
}

static inline std::string_view debug_geom = "#extension GL_EXT_geometry_shader4: enable\n"
"\n"
"layout(triangles) in;\n"
"in float adjusted[];"
"layout(triangle_strip, max_vertices = 3) out;\n"
"out float adjusted_b;"
"\n"
"vec3 V[3];\n"
"vec3 CG;\n"
"\n"
"void ProduceVertex(int v) {\n"
"	gl_Position = vec4(CG + 0.95f * (V[v] - CG), 1.0f);\n"
"	adjusted_b = adjusted[v];\n"
"	EmitVertex();\n"
"}\n"
"\n"
"void main() {\n"
"	V[0] = gl_PositionIn[0].xyz;\n"
"	V[1] = gl_PositionIn[1].xyz;\n"
"	V[2] = gl_PositionIn[2].xyz;\n"
"	CG = (V[0] + V[1] + V[2]) / 3.0f;\n"
"	ProduceVertex(0);\n"
"	ProduceVertex(1);\n"
"	ProduceVertex(2);\n"
"}";

GLuint create_program(std::string_view vertex_shader, std::string_view tes_control_shader, std::string_view tes_eval_shader, std::string_view fragment_shader, bool debug_geom_shader) {
	GLuint return_value = glCreateProgram();
	if(return_value == 0) {
		notify_user_of_fatal_opengl_error("program creation failed");
	}

	auto v_shader = compile_shader(vertex_shader, GL_VERTEX_SHADER);
	auto f_shader = compile_shader(fragment_shader, GL_FRAGMENT_SHADER);
	auto tc_shader = compile_shader(tes_control_shader, GL_TESS_CONTROL_SHADER);
	auto te_shader = compile_shader(tes_eval_shader, GL_TESS_EVALUATION_SHADER);

	glAttachShader(return_value, v_shader);
	glAttachShader(return_value, tc_shader);
	glAttachShader(return_value, te_shader);
	glAttachShader(return_value, f_shader);

	if(debug_geom_shader) {
		auto g_shader = compile_shader(debug_geom, GL_GEOMETRY_SHADER);
		glAttachShader(return_value, g_shader);
	}

	glLinkProgram(return_value);

	GLint result;
	glGetProgramiv(return_value, GL_LINK_STATUS, &result);
	if(result == GL_FALSE) {
		GLint logLen;
		glGetProgramiv(return_value, GL_INFO_LOG_LENGTH, &logLen);

		char* log = new char[static_cast<size_t>(logLen)];
		GLsizei written;
		glGetProgramInfoLog(return_value, logLen, &written, log);
		notify_user_of_fatal_opengl_error(std::string("Program failed to link:\n") + log);
	}

	glDeleteShader(v_shader);
	glDeleteShader(f_shader);

	return return_value;
}

void load_special_icons(ogl::data& state, simple_fs::file_system& fs) {
	auto root = get_root(fs);
	auto gfx_dir = simple_fs::open_directory(root, NATIVE("gfx"));

	auto interface_dir = simple_fs::open_directory(gfx_dir, NATIVE("interface"));
	auto money_dds = simple_fs::open_file(interface_dir, NATIVE("icon_money_big.dds"));
	if(money_dds) {
		auto content = simple_fs::view_contents(*money_dds);
		uint32_t size_x, size_y;
		state.money_icon_tex = GLuint(ogl::SOIL_direct_load_DDS_from_memory(reinterpret_cast<uint8_t const*>(content.data),
				content.file_size, size_x, size_y, ogl::SOIL_FLAG_TEXTURE_REPEATS));
	}

	auto assets_dir = simple_fs::open_directory(root, NATIVE("assets"));
	auto cross_dds = simple_fs::open_file(assets_dir, NATIVE("trigger_not.dds"));
	if(cross_dds) {
		auto content = simple_fs::view_contents(*cross_dds);
		uint32_t size_x, size_y;
		state.cross_icon_tex = GLuint(ogl::SOIL_direct_load_DDS_from_memory(reinterpret_cast<uint8_t const*>(content.data),
				content.file_size, size_x, size_y, ogl::SOIL_FLAG_TEXTURE_REPEATS));
	}
	auto cross_desat_dds = simple_fs::open_file(assets_dir, NATIVE("trigger_not_desaturated.dds"));
	if(cross_desat_dds) {
		auto content = simple_fs::view_contents(*cross_desat_dds);
		uint32_t size_x, size_y;
		state.cross_desaturated_icon_tex = GLuint(ogl::SOIL_direct_load_DDS_from_memory(reinterpret_cast<uint8_t const*>(content.data),
				content.file_size, size_x, size_y, ogl::SOIL_FLAG_TEXTURE_REPEATS));
	}
	auto cb_cross_dds = simple_fs::open_file(assets_dir, NATIVE("trigger_not_cb.dds"));
	if(cb_cross_dds) {
		auto content = simple_fs::view_contents(*cb_cross_dds);
		uint32_t size_x, size_y;
		state.color_blind_cross_icon_tex = GLuint(ogl::SOIL_direct_load_DDS_from_memory(reinterpret_cast<uint8_t const*>(content.data),
			content.file_size, size_x, size_y, ogl::SOIL_FLAG_TEXTURE_REPEATS));
	}
	auto checkmark_dds = simple_fs::open_file(assets_dir, NATIVE("trigger_yes.dds"));
	if(checkmark_dds) {
		auto content = simple_fs::view_contents(*checkmark_dds);
		uint32_t size_x, size_y;
		state.checkmark_icon_tex = GLuint(ogl::SOIL_direct_load_DDS_from_memory(
				reinterpret_cast<uint8_t const*>(content.data), content.file_size, size_x, size_y, ogl::SOIL_FLAG_TEXTURE_REPEATS));
	}
	auto checkmark_desat_dds = simple_fs::open_file(assets_dir, NATIVE("trigger_yes_desaturated.dds"));
	if(checkmark_desat_dds) {
		auto content = simple_fs::view_contents(*checkmark_desat_dds);
		uint32_t size_x, size_y;
		state.checkmark_desaturated_icon_tex = GLuint(ogl::SOIL_direct_load_DDS_from_memory(
				reinterpret_cast<uint8_t const*>(content.data), content.file_size, size_x, size_y, ogl::SOIL_FLAG_TEXTURE_REPEATS));
	}

	auto n_dds = simple_fs::open_file(interface_dir, NATIVE("politics_foreign_naval_units.dds"));
	if(n_dds) {
		auto content = simple_fs::view_contents(*n_dds);
		uint32_t size_x, size_y;
		state.navy_icon_tex = GLuint(ogl::SOIL_direct_load_DDS_from_memory(
			reinterpret_cast<uint8_t const*>(content.data), content.file_size, size_x, size_y, ogl::SOIL_FLAG_TEXTURE_REPEATS));
	}
	auto a_dds = simple_fs::open_file(interface_dir, NATIVE("topbar_army.dds"));
	if(a_dds) {
		auto content = simple_fs::view_contents(*a_dds);
		uint32_t size_x, size_y;
		state.army_icon_tex = GLuint(ogl::SOIL_direct_load_DDS_from_memory(
			reinterpret_cast<uint8_t const*>(content.data), content.file_size, size_x, size_y, ogl::SOIL_FLAG_TEXTURE_REPEATS));
	}
}

std::string_view framebuffer_error(GLenum e) {
	switch(e) {
	case GL_FRAMEBUFFER_UNDEFINED:
		return "GL_FRAMEBUFFER_UNDEFINED";
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT ";
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
	case GL_FRAMEBUFFER_UNSUPPORTED:
		return "GL_FRAMEBUFFER_UNSUPPORTED";
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
	default:
		break;
	}
	return "???";
}

void initialize_framebuffer_for_province_indices(ogl::data& state, int32_t size_x, int32_t size_y) {
	if(!size_x || !size_y)
		return;
	// prepare textures for rendering
	glGenTextures(1, &state.province_map_rendertexture);
	glBindTexture(GL_TEXTURE_2D, state.province_map_rendertexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size_x, size_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenRenderbuffers(1, &state.province_map_depthbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, state.province_map_depthbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size_x, size_y);

	// framebuffer
	glGenFramebuffers(1, &state.province_map_framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state.province_map_framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.province_map_rendertexture, 0);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, state.province_map_depthbuffer);

	// drawbuffers
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);

	auto check = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

}

void deinitialize_framebuffer_for_province_indices(ogl::data& state) {
	if(state.province_map_rendertexture)
		glDeleteTextures(1, &state.province_map_rendertexture);
	if(state.province_map_depthbuffer)
		glDeleteRenderbuffers(1, &state.province_map_depthbuffer);
	if(state.province_map_framebuffer)
		glDeleteFramebuffers(1, &state.province_map_framebuffer);
	//state.console_log(ogl::opengl_get_error_name(glGetError()));
}

void initialize_msaa(ogl::data& state, simple_fs::file_system& fs, int32_t size_x, int32_t size_y) {
	//if(state.user_settings.antialias_level == 0)
	//	return;

	GLsizei antialias_level = 1;

	if(!size_x || !size_y)
		return;
	glEnable(GL_MULTISAMPLE);
	// setup screen VAO
	static const float sq_vertices[] = {
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};
	glGenVertexArrays(1, &state.msaa_vao);
	glGenBuffers(1, &state.msaa_vbo);
	glBindVertexArray(state.msaa_vao);
	glBindBuffer(GL_ARRAY_BUFFER, state.msaa_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sq_vertices), &sq_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	// framebuffer
	glGenFramebuffers(1, &state.msaa_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, state.msaa_framebuffer);
	// create a multisampled color attachment texture
	glGenTextures(1, &state.msaa_texcolorbuffer);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, state.msaa_texcolorbuffer);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, GLsizei(antialias_level), GL_RGBA, size_x, size_y, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, state.msaa_texcolorbuffer, 0);
	// create a (also multisampled) renderbuffer object for depth and stencil attachments
	glGenRenderbuffers(1, &state.msaa_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, state.msaa_rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, GLsizei(antialias_level), GL_DEPTH24_STENCIL8, size_x, size_y);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, state.msaa_rbo);
	if(auto r = glCheckFramebufferStatus(GL_FRAMEBUFFER); r != GL_FRAMEBUFFER_COMPLETE) {
		state.msaa_enabled = false;
		return;
	}
	// configure second post-processing framebuffer
	glGenFramebuffers(1, &state.msaa_interbuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, state.msaa_interbuffer);
	// create a color attachment texture
	glGenTextures(1, &state.msaa_texture);
	glBindTexture(GL_TEXTURE_2D, state.msaa_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size_x, size_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.msaa_texture, 0);	// we only need a color buffer
	if(auto r = glCheckFramebufferStatus(GL_FRAMEBUFFER); r != GL_FRAMEBUFFER_COMPLETE) {
		notify_user_of_fatal_opengl_error("MSAA post processing framebuffer wasn't completed: " + std::string(framebuffer_error(r)));
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	auto root = get_root(fs);
	auto msaa_fshader = open_file(root, NATIVE("assets/shaders/glsl/msaa_f_shader.glsl"));
	auto msaa_vshader = open_file(root, NATIVE("assets/shaders/glsl/msaa_v_shader.glsl"));
	if(bool(msaa_fshader) && bool(msaa_vshader)) {
		auto vertex_content = view_contents(*msaa_vshader);
		auto fragment_content = view_contents(*msaa_fshader);
		state.msaa_shader_program = create_program(std::string_view(vertex_content.data, vertex_content.file_size), std::string_view(fragment_content.data, fragment_content.file_size));
		state.msaa_uniform_screen_size = glGetUniformLocation(state.msaa_shader_program, "screen_size");
		state.msaa_uniform_gaussian_blur = glGetUniformLocation(state.msaa_shader_program, "gaussian_radius");
	} else {
		notify_user_of_fatal_opengl_error("Unable to open a MSAA shaders file");
	}
	state.msaa_enabled = true;
}


void deinitialize_msaa(ogl::data& state) {
	if(!state.msaa_enabled)
		return;

	state.msaa_enabled = false;
	if(state.msaa_texture)
		glDeleteTextures(1, &state.msaa_texture);
	if(state.msaa_interbuffer)
		glDeleteFramebuffers(1, &state.msaa_interbuffer);
	if(state.msaa_rbo)
		glDeleteRenderbuffers(1, &state.msaa_rbo);
	if(state.msaa_texcolorbuffer)
		glDeleteTextures(1, &state.msaa_texcolorbuffer);
	if(state.msaa_framebuffer)
		glDeleteFramebuffers(1, &state.msaa_framebuffer);
	if(state.msaa_vbo)
		glDeleteBuffers(1, &state.msaa_vbo);
	if(state.msaa_vao)
		glDeleteVertexArrays(1, &state.msaa_vao);
	if(state.msaa_shader_program)
		glDeleteProgram(state.msaa_shader_program);
	glDisable(GL_MULTISAMPLE);
}

static const GLfloat global_square_data[] = {
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 1.0f, 0.0f
};
static const GLfloat global_square_right_data[] = {
	0.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 0.0f,
	1.0f, 0.0f, 0.0f, 0.0f
};
static const GLfloat global_square_left_data[] = {
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f, 1.0f
};
static const GLfloat global_square_flipped_data[] = {
	0.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 1.0f, 0.0f,
	1.0f, 0.0f, 1.0f, 1.0f
};
static const GLfloat global_square_right_flipped_data[] = {
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 1.0f
};
static const GLfloat global_square_left_flipped_data[] = {
	0.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 1.0f, 0.0f
};

//RTL squares
static const GLfloat global_rtl_square_data[] = {
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 0.0f
};
static const GLfloat global_rtl_square_right_data[] = {
	0.0f, 1.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f
};
static const GLfloat global_rtl_square_left_data[] = {
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 1.0f
};
static const GLfloat global_rtl_square_flipped_data[] = {
	0.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f, 1.0f
};
static const GLfloat global_rtl_square_right_flipped_data[] = {
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f, 1.0f
};
static const GLfloat global_rtl_square_left_flipped_data[] = {
	0.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 0.0f,
	1.0f, 0.0f, 0.0f, 0.0f
};

void load_shaders(ogl::data& state, simple_fs::file_system& fs) {
	auto root = get_root(fs);
	auto ui_fshader = open_file(root, NATIVE("assets/shaders/glsl/ui_f_shader.glsl"));
	auto ui_vshader = open_file(root, NATIVE("assets/shaders/glsl/ui_v_shader.glsl"));
	if(bool(ui_fshader) && bool(ui_vshader)) {
		auto vertex_content = view_contents(*ui_vshader);
		auto fragment_content = view_contents(*ui_fshader);
		state.ui_shader_program = create_program(std::string_view(vertex_content.data, vertex_content.file_size), std::string_view(fragment_content.data, fragment_content.file_size));

		state.ui_shader_texture_sampler_uniform = glGetUniformLocation(state.ui_shader_program, "texture_sampler");
		state.ui_shader_secondary_texture_sampler_uniform = glGetUniformLocation(state.ui_shader_program, "secondary_texture_sampler");
		state.ui_shader_screen_width_uniform = glGetUniformLocation(state.ui_shader_program, "screen_width");
		state.ui_shader_screen_height_uniform = glGetUniformLocation(state.ui_shader_program, "screen_height");
		state.ui_shader_gamma_uniform = glGetUniformLocation(state.ui_shader_program, "gamma");

		state.ui_shader_d_rect_uniform = glGetUniformLocation(state.ui_shader_program, "d_rect");
		state.ui_shader_subroutines_index_uniform = glGetUniformLocation(state.ui_shader_program, "subroutines_index");
		state.ui_shader_inner_color_uniform = glGetUniformLocation(state.ui_shader_program, "inner_color");
		state.ui_shader_subrect_uniform = glGetUniformLocation(state.ui_shader_program, "subrect");
		state.ui_shader_border_size_uniform = glGetUniformLocation(state.ui_shader_program, "border_size");
	} else {
		notify_user_of_fatal_opengl_error("Unable to open a necessary shader file");
	}
}

void load_global_squares(ogl::data& state) {
	// Populate the position buffer
	glGenBuffers(1, &state.global_square_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state.global_square_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_square_data, GL_STATIC_DRAW);
	//RTL version
	glGenBuffers(1, &state.global_rtl_square_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state.global_rtl_square_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_rtl_square_data, GL_STATIC_DRAW);

	glGenVertexArrays(1, &state.global_square_vao);
	glBindVertexArray(state.global_square_vao);
	glEnableVertexAttribArray(0); // position
	glEnableVertexAttribArray(1); // texture coordinates

	glBindVertexBuffer(0, state.global_square_buffer, 0, sizeof(GLfloat) * 4);

	glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);									 // position
	glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2); // texture coordinates
	glVertexAttribBinding(0, 0);																				 // position -> to array zero
	glVertexAttribBinding(1, 0);																				 // texture coordinates -> to array zero

	glGenBuffers(1, &state.global_square_left_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state.global_square_left_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_square_left_data, GL_STATIC_DRAW);

	glGenBuffers(1, &state.global_square_right_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state.global_square_right_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_square_right_data, GL_STATIC_DRAW);

	glGenBuffers(1, &state.global_square_right_flipped_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state.global_square_right_flipped_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_square_right_flipped_data, GL_STATIC_DRAW);

	glGenBuffers(1, &state.global_square_left_flipped_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state.global_square_left_flipped_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_square_left_flipped_data, GL_STATIC_DRAW);

	glGenBuffers(1, &state.global_square_flipped_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state.global_square_flipped_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_square_flipped_data, GL_STATIC_DRAW);

	//RTL mode squares
	glGenBuffers(1, &state.global_rtl_square_left_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state.global_rtl_square_left_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_rtl_square_left_data, GL_STATIC_DRAW);

	glGenBuffers(1, &state.global_rtl_square_right_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state.global_rtl_square_right_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_rtl_square_right_data, GL_STATIC_DRAW);

	glGenBuffers(1, &state.global_rtl_square_right_flipped_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state.global_rtl_square_right_flipped_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_rtl_square_right_flipped_data, GL_STATIC_DRAW);

	glGenBuffers(1, &state.global_rtl_square_left_flipped_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state.global_rtl_square_left_flipped_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_rtl_square_left_flipped_data, GL_STATIC_DRAW);

	glGenBuffers(1, &state.global_rtl_square_flipped_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state.global_rtl_square_flipped_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_rtl_square_flipped_data, GL_STATIC_DRAW);
}

void bind_vertices_by_rotation(ogl::data const& state, ui::rotation r, bool flipped, bool rtl) {
	switch(r) {
	case ui::rotation::upright:
		if(!flipped)
			glBindVertexBuffer(
				0,
				rtl
					? state.global_rtl_square_buffer
					: state.global_square_buffer,
				0,
				sizeof(GLfloat) * 4
			);
		else
			glBindVertexBuffer(0, rtl ? state.global_rtl_square_flipped_buffer : state.global_square_flipped_buffer, 0, sizeof(GLfloat) * 4);
		break;
	case ui::rotation::r90_left:
		if(!flipped)
			glBindVertexBuffer(0, rtl ? state.global_rtl_square_left_buffer: state.global_square_left_buffer, 0, sizeof(GLfloat) * 4);
		else
			glBindVertexBuffer(0, rtl ? state.global_rtl_square_left_flipped_buffer : state.global_square_left_flipped_buffer, 0, sizeof(GLfloat) * 4);
		break;
	case ui::rotation::r90_right:
		if(!flipped)
			glBindVertexBuffer(0, rtl ? state.global_rtl_square_right_buffer : state.global_square_right_buffer, 0, sizeof(GLfloat) * 4);
		else
			glBindVertexBuffer(0, rtl ? state.global_rtl_square_right_flipped_buffer : state.global_square_right_flipped_buffer, 0, sizeof(GLfloat) * 4);
		break;
	}
}

void render_colored_rect(
	ogl::data const& state,
	float x, float y, float width, float height,
	float red, float green, float blue,
	ui::rotation r, bool flipped, bool rtl
) {
	glBindVertexArray(state.global_square_vao);
	bind_vertices_by_rotation(state, r, flipped, rtl);
	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);
	GLuint subroutines[2] = { map_color_modification_to_index(color_modification::none), parameters::solid_color };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	glUniform3f(state.ui_shader_inner_color_uniform, red, green, blue);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call
	glLineWidth(2.0f);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_alpha_colored_rect(
	ogl::data const& state,
	float x, float y, float width, float height,
	float red, float green, float blue, float alpha
) {
	glBindVertexArray(state.global_square_vao);
	glBindVertexBuffer(0, state.global_square_buffer, 0, sizeof(GLfloat) * 4);
	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);
	GLuint subroutines[2] = { map_color_modification_to_index(color_modification::none), parameters::alpha_color };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	glUniform3f(state.ui_shader_inner_color_uniform, red, green, blue);
	glUniform1f(state.ui_shader_border_size_uniform, alpha);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call
	glLineWidth(2.0f);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_simple_rect(ogl::data const& state, float x, float y, float width, float height, ui::rotation r, bool flipped, bool rtl) {
	render_colored_rect(
		state, x, y, width, height, 1.0f, 1.0f, 1.0f, r, flipped, rtl
	);
}

void render_textured_rect(ogl::data const& state, color_modification enabled, float x, float y, float width, float height,
		GLuint texture_handle, ui::rotation r, bool flipped, bool rtl) {
	glBindVertexArray(state.global_square_vao);

	bind_vertices_by_rotation(state, r, flipped, rtl);

	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);
	// glUniform4f(state.ui_shader_d_rect_uniform, 0, 0, width, height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_handle);

	GLuint subroutines[2] = {map_color_modification_to_index(enabled), parameters::no_filter};
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_textured_rect_direct(ogl::data const& state, float x, float y, float width, float height, uint32_t handle) {
	glBindVertexArray(state.global_square_vao);

	glBindVertexBuffer(0, state.global_square_buffer, 0, sizeof(GLfloat) * 4);

	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, handle);

	GLuint subroutines[2] = {parameters::enabled, parameters::no_filter};
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_ui_mesh(
	ogl::data const& state,
	color_modification enabled,
	float x, float y,
	float width, float height,
	generic_ui_mesh_triangle_strip& mesh,
	data_texture& t
) {
	glBindVertexArray(state.global_square_vao);

	mesh.bind_buffer();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, t.handle());

	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);
	GLuint subroutines[2] = { map_color_modification_to_index(enabled), parameters::triangle_strip };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(mesh.count));
}

void render_linegraph(ogl::data const& state, color_modification enabled, float x, float y, float width, float height,
		lines& l) {
	glBindVertexArray(state.global_square_vao);

	l.bind_buffer();

	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);
	GLuint subroutines[2] = { map_color_modification_to_index(enabled), parameters::linegraph };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call


	glLineWidth(2.0f);

	glUniform3f(state.ui_shader_inner_color_uniform, 1.f, 1.f, 0.f);
	glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(l.count));
}

void render_linegraph(ogl::data const& state, color_modification enabled, float x, float y, float width, float height, float r, float g, float b,
		lines& l) {
	glBindVertexArray(state.global_square_vao);

	l.bind_buffer();

	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);
	GLuint subroutines[2] = { map_color_modification_to_index(enabled), parameters::linegraph_color };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glLineWidth(2.0f);
	glUniform3f(state.ui_shader_inner_color_uniform, r, g, b);
	glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(l.count));
}

void render_linegraph(
	ogl::data const& state,
	color_modification enabled,
	float x, float y, float width, float height, float r, float g, float b, float a, lines& l,
	float ui_scale
) {
	glBindVertexArray(state.global_square_vao);

	l.bind_buffer();

	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);
	GLuint subroutines[2] = { map_color_modification_to_index(enabled), parameters::linegraph_acolor };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glUniform1f(state.ui_shader_border_size_uniform, a);

	glLineWidth(2.0f * ui_scale);
	glUniform3f(state.ui_shader_inner_color_uniform, r, g, b);
	glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(l.count));
}

void render_barchart(ogl::data const& state, color_modification enabled, float x, float y, float width, float height,
		data_texture& t, ui::rotation r, bool flipped, bool rtl) {
	glBindVertexArray(state.global_square_vao);

	bind_vertices_by_rotation(state, r, flipped, rtl);

	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, t.handle());

	GLuint subroutines[2] = {map_color_modification_to_index(enabled), parameters::barchart};
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_piechart(ogl::data const& state, color_modification enabled, float x, float y, float size, data_texture& t) {
	glBindVertexArray(state.global_square_vao);

	glBindVertexBuffer(0, state.global_square_buffer, 0, sizeof(GLfloat) * 4);

	glUniform4f(state.ui_shader_d_rect_uniform, x, y, size, size);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, t.handle());

	GLuint subroutines[2] = {map_color_modification_to_index(enabled), parameters::piechart};
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
void render_stripchart(ogl::data const& state, color_modification enabled, float x, float y, float sizex, float sizey, data_texture& t) {
	glBindVertexArray(state.global_square_vao);

	glBindVertexBuffer(0, state.global_square_buffer, 0, sizeof(GLfloat) * 4);

	glUniform4f(state.ui_shader_d_rect_uniform, x, y, sizex, sizey);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, t.handle());

	GLuint subroutines[2] = { map_color_modification_to_index(enabled), parameters::stripchart };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
void render_bordered_rect(ogl::data const& state, color_modification enabled, float border_size, float x, float y, float width,
		float height, GLuint texture_handle, ui::rotation r, bool flipped, bool rtl) {
	glBindVertexArray(state.global_square_vao);

	bind_vertices_by_rotation(state, r, flipped, rtl);

	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);
	glUniform1f(state.ui_shader_border_size_uniform, border_size);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_handle);

	GLuint subroutines[2] = {map_color_modification_to_index(enabled), parameters::frame_stretch};
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}


void render_rect_with_repeated_border(ogl::data const& state, color_modification enabled, float grid_size, float x, float y, float width,
		float height, GLuint texture_handle, ui::rotation r, bool flipped, bool rtl) {
	glBindVertexArray(state.global_square_vao);
	bind_vertices_by_rotation(state, r, flipped, rtl);
	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);
	glUniform1f(state.ui_shader_border_size_uniform, grid_size);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_handle);
	GLuint subroutines[2] = { map_color_modification_to_index(enabled), parameters::border_repeat };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_rect_with_repeated_corner(ogl::data const& state, color_modification enabled, float grid_size, float x, float y, float width,
		float height, GLuint texture_handle, ui::rotation r, bool flipped, bool rtl) {
	glBindVertexArray(state.global_square_vao);
	bind_vertices_by_rotation(state, r, flipped, rtl);
	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);
	glUniform1f(state.ui_shader_border_size_uniform, grid_size);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_handle);
	GLuint subroutines[2] = { map_color_modification_to_index(enabled), parameters::corner_repeat };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_masked_rect(ogl::data const& state, color_modification enabled, float x, float y, float width, float height,
		GLuint texture_handle, GLuint mask_texture_handle, ui::rotation r, bool flipped, bool rtl) {
	glBindVertexArray(state.global_square_vao);

	bind_vertices_by_rotation(state, r, flipped, rtl);

	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_handle);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mask_texture_handle);

	GLuint subroutines[2] = {map_color_modification_to_index(enabled), parameters::use_mask};
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_progress_bar(ogl::data const& state, color_modification enabled, float progress, float x, float y, float width,
		float height, GLuint left_texture_handle, GLuint right_texture_handle, ui::rotation r, bool flipped, bool rtl) {
	glBindVertexArray(state.global_square_vao);

	bind_vertices_by_rotation(state, r, flipped, rtl);

	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);
	glUniform1f(state.ui_shader_border_size_uniform, progress);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, left_texture_handle);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, right_texture_handle);

	GLuint subroutines[2] = {map_color_modification_to_index(enabled), parameters::progress_bar};
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_tinted_textured_rect(ogl::data const& state, float x, float y, float width, float height, float r, float g, float b,
		GLuint texture_handle, ui::rotation rot, bool flipped, bool rtl) {
	glBindVertexArray(state.global_square_vao);

	bind_vertices_by_rotation(state, rot, flipped, rtl);

	glUniform3f(state.ui_shader_inner_color_uniform, r, g, b);
	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_handle);

	GLuint subroutines[2] = {parameters::tint, parameters::no_filter};
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_tinted_rect(
	ogl::data const& state,
	float x, float y, float width, float height,
	float r, float g, float b,
	ui::rotation rot, bool flipped, bool rtl
) {
	glBindVertexArray(state.global_square_vao);
	bind_vertices_by_rotation(state, rot, flipped, rtl);
	glUniform3f(state.ui_shader_inner_color_uniform, r, g, b);
	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);
	GLuint subroutines[2] = { parameters::tint, parameters::transparent_color };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_tinted_subsprite(ogl::data const& state, int frame, int total_frames, float x, float y,
		float width, float height, float r, float g, float b, GLuint texture_handle, ui::rotation rot, bool flipped,
		bool rtl) {
	glBindVertexArray(state.global_square_vao);

	bind_vertices_by_rotation(state, rot, flipped, rtl);

	auto const scale = 1.0f / static_cast<float>(total_frames);
	glUniform3f(state.ui_shader_inner_color_uniform, static_cast<float>(frame) * scale, scale, 0.0f);
	glUniform4f(state.ui_shader_subrect_uniform, r, g, b, 0);
	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_handle);

	GLuint subroutines[2] = { parameters::alternate_tint, parameters::sub_sprite };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_subsprite(ogl::data const& state, color_modification enabled, int frame, int total_frames, float x, float y,
		float width, float height, GLuint texture_handle, ui::rotation r, bool flipped, bool rtl) {
	glBindVertexArray(state.global_square_vao);

	bind_vertices_by_rotation(state, r, flipped, rtl);

	auto const scale = 1.0f / static_cast<float>(total_frames);
	glUniform3f(state.ui_shader_inner_color_uniform, static_cast<float>(frame) * scale, scale, 0.0f);
	glUniform4f(state.ui_shader_d_rect_uniform, x, y, width, height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_handle);

	GLuint subroutines[2] = {map_color_modification_to_index(enabled), parameters::sub_sprite};
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
void render_rect_slice(ogl::data& state, float x, float y, float width, float height, GLuint texture_handle, float start_slice, float end_slice) {
	glBindVertexArray(state.global_square_vao);

	bind_vertices_by_rotation(state, ui::rotation::upright, false, false);

	glUniform3f(state.ui_shader_inner_color_uniform, start_slice, end_slice - start_slice, 0.0f);
	glUniform4f(state.ui_shader_d_rect_uniform, x + width * start_slice, y, width * (end_slice - start_slice), height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_handle);

	GLuint subroutines[2] = { map_color_modification_to_index(color_modification::none), parameters::sub_sprite };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}


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
) {
	float scale = 1.f;
	float icon_baseline = baseline_y + (
		f	.retrieve_instance(font_collection, int32_t(font_size), ui_scale)
			.ascender(ui_scale)
	) - font_size;

	bind_vertices_by_rotation(state, ui::rotation::upright, false, false);
	glActiveTexture(GL_TEXTURE0);

	switch(ico) {
	case text::embedded_icon::check:
		glBindTexture(GL_TEXTURE_2D, state.checkmark_icon_tex);
		icon_baseline += font_size * 0.1f;
		break;
	case text::embedded_icon::xmark:
	{
		GLuint false_icon = state.cross_icon_tex;
		glBindTexture(GL_TEXTURE_2D, false_icon);
		icon_baseline += font_size * 0.1f;
		break;
	} case text::embedded_icon::xmark_desaturated:
	{
		GLuint false_icon = state.cross_desaturated_icon_tex;
		glBindTexture(GL_TEXTURE_2D, false_icon);
		icon_baseline += font_size * 0.1f;
		break;
	} case text::embedded_icon::check_desaturated:
	{
		GLuint false_icon = state.checkmark_desaturated_icon_tex;
		glBindTexture(GL_TEXTURE_2D, false_icon);
		icon_baseline += font_size * 0.1f;
		break;
	}
	}

	GLuint icon_subroutines[2] = { map_color_modification_to_index(cmod), parameters::no_filter };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, icon_subroutines[0], icon_subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, icon_subroutines); // must set all subroutines in one call
	glUniform4f(state.ui_shader_d_rect_uniform, x, icon_baseline, scale * font_size, scale * font_size);
	glUniform4f(state.ui_shader_subrect_uniform, 0.f, 1.f, 0.f, 1.f);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void text_render(
	FT_Library lib,
	GLuint square_buffer,
	float ui_scale,
	GLuint ui_shader_subroutines_index_uniform,
	unsigned int subroutine_1,
	unsigned int subroutine_2,
	GLuint ui_shader_d_rect_uniform,
	GLuint ui_shader_subrect_uniform,
	const std::vector<text::stored_glyph>& glyph_info,
	unsigned int glyph_count,
	float x,
	float baseline_y,
	float size,
	text::font& f
) {
	glBindVertexBuffer(0, square_buffer, 0, sizeof(GLfloat) * 4);
	glUniform2ui(ui_shader_subroutines_index_uniform, subroutine_1, subroutine_2);

	auto& font_instance = f.retrieve_stateless_instance(lib, int32_t(size * ui_scale));

	x = std::floor(x * ui_scale);
	baseline_y = std::floor(baseline_y * ui_scale);

	for(unsigned int i = 0; i < glyph_count; i++) {
		hb_codepoint_t glyphid = glyph_info[i].codepoint;

		auto pixel_x_off = x + float(glyph_info[i].x_offset) / text::fixed_to_fp;
		auto trunc_pixel_x_off = std::floor(pixel_x_off);
		auto frac_pixel_off = pixel_x_off - trunc_pixel_x_off;

		int32_t subpixel = 0;
		pixel_x_off = (trunc_pixel_x_off);

		if(frac_pixel_off < 0.125f) {

		} else if(frac_pixel_off < 0.375f) {
			subpixel = 1;
		} else if(frac_pixel_off < 0.625f) {
			subpixel = 2;
		} else if(frac_pixel_off < 0.875f) {
			subpixel = 3;
		} else {
			pixel_x_off = trunc_pixel_x_off + 1.0f;
		}

		font_instance.make_glyph(uint16_t(glyphid), subpixel);
		auto& gso = font_instance.get_glyph(uint16_t(glyphid), subpixel);
		float x_advance = float(glyph_info[i].x_advance) / text::fixed_to_fp;

		if(gso.width != 0) {
			float x_offset = pixel_x_off + float(gso.bitmap_left);
			float y_offset = float(-gso.bitmap_top) - float(glyph_info[i].y_offset) / text::fixed_to_fp;

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, font_instance.textures[gso.tx_sheet]);

			glUniform4f(ui_shader_d_rect_uniform, x_offset / ui_scale, (baseline_y + y_offset) / ui_scale, float(gso.width) / ui_scale, float(gso.height) / ui_scale);
			glUniform4f(ui_shader_subrect_uniform, float(gso.x) / float(1024) /* x offset */,
					float(gso.width) / float(1024) /* x width */, float(gso.y) / float(1024) /* y offset */,
					float(gso.height) / float(1024) /* y height */
			);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		x += x_advance;
		baseline_y -= (float(glyph_info[i].y_advance) / text::fixed_to_fp);
	}
}



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
) {
	glUniform3f(state.ui_shader_inner_color_uniform, c.r, c.g, c.b);
	glUniform1f(state.ui_shader_border_size_uniform, 0.08f * 16.0f / size);
	text_render(
		font_collection.ft_library,
		state.global_square_buffer,
		ui_scale,
		state.ui_shader_subroutines_index_uniform,
		map_color_modification_to_index(enabled),
		ogl::parameters::subsprite_b,
		state.ui_shader_d_rect_uniform,
		state.ui_shader_subrect_uniform,
		txt.glyph_info,
		static_cast<unsigned int>(txt.glyph_info.size()),
		x,
		y + size,
		size,
		f
	);
}

void lines::set_y(float* v) {
	for(int32_t i = 0; i < static_cast<int32_t>(count); ++i) {
		buffer[i * 4] = static_cast<float>(i) / static_cast<float>(count - 1);
		buffer[i * 4 + 1] = 1.0f - v[i];
		buffer[i * 4 + 2] = 0.5f;
		buffer[i * 4 + 3] = v[i];
	}
	pending_data_update = true;
}

void lines::set_default_y() {
	for(int32_t i = 0; i < static_cast<int32_t>(count); ++i) {
		buffer[i * 4] = static_cast<float>(i) / static_cast<float>(count - 1);
		buffer[i * 4 + 1] = 0.5f;
		buffer[i * 4 + 2] = 0.5f;
		buffer[i * 4 + 3] = 0.5f;
	}
	pending_data_update = true;
}

void lines::bind_buffer() {
	if(buffer_handle == 0) {
		glGenBuffers(1, &buffer_handle);

		glBindBuffer(GL_ARRAY_BUFFER, buffer_handle);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * count * 4, nullptr, GL_DYNAMIC_DRAW);
	}
	if(buffer && pending_data_update) {
		glBindBuffer(GL_ARRAY_BUFFER, buffer_handle);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * count * 4, buffer);
		pending_data_update = false;
	}

	glBindVertexBuffer(0, buffer_handle, 0, sizeof(GLfloat) * 4);
}

void generic_ui_mesh_triangle_strip::set_coords(float* v) {
	for(int32_t i = 0; i < static_cast<int32_t>(count); ++i) {
		// coords
		buffer[i * 4 + 0]	= 0.5f + v[2 * i] * 0.5f;
		buffer[i * 4 + 1]	= 0.5f + v[2 * i + 1] * 0.5f;

		// texcoords
		buffer[i * 4 + 2]	= static_cast<float>(i) / static_cast<float>(count - 1);
		buffer[i * 4 + 3]	= 0.5f;
	}
	pending_data_update = true;
}

void generic_ui_mesh_triangle_strip::set_default() {
	// set circle by default

	for(int32_t i = 0; i < static_cast<int32_t>(count); ++i) {
		float frac = static_cast<float>(i / 2) / static_cast<float>((count - 1) / 2);
		float t = frac * std::numbers::pi_v<float> * 2.f;

		if(i % 2 == 0) {
			// inner

			// coords
			buffer[i * 4 + 0]	= 0.5f + std::cos(t) * 0.3f;
			buffer[i * 4 + 1]	= 0.5f + std::sin(t) * 0.3f;
			// texcoords
			buffer[i * 4 + 2]	= frac;
			buffer[i * 4 + 3]	= 0.f;
		} else {
			// outer

			// coords
			buffer[i * 4 + 0]	= 0.5f + std::cos(t) * 0.5f;
			buffer[i * 4 + 1]	= 0.5f + std::sin(t) * 0.5f;
			// texcoords
			buffer[i * 4 + 2]	= frac;
			buffer[i * 4 + 3]	= 1.f;
		}
	}
	pending_data_update = true;
}

void generic_ui_mesh_triangle_strip::bind_buffer() {
	if(buffer_handle == 0) {
		glGenBuffers(1, &buffer_handle);

		glBindBuffer(GL_ARRAY_BUFFER, buffer_handle);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * count * 4, nullptr, GL_DYNAMIC_DRAW);
	}
	if(buffer && pending_data_update) {
		glBindBuffer(GL_ARRAY_BUFFER, buffer_handle);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * count * 4, buffer);
		pending_data_update = false;
	}

	glBindVertexBuffer(0, buffer_handle, 0, sizeof(GLfloat) * 4);
}

bool msaa_enabled(ogl::data const& state) {
	return state.msaa_enabled;
}

image load_stb_image(simple_fs::file& file) {
	int32_t file_channels = 4;
	int32_t size_x = 0;
	int32_t size_y = 0;
	auto content = simple_fs::view_contents(file);
	auto data = stbi_load_from_memory(reinterpret_cast<uint8_t const*>(content.data), int32_t(content.file_size), &size_x, &size_y, &file_channels, 4);
	return image(data, size_x, size_y, 4);
}

GLuint make_gl_texture(uint8_t* data, uint32_t size_x, uint32_t size_y, uint32_t channels) {
	GLuint texture_handle;
	glGenTextures(1, &texture_handle);
	const GLuint internalformats[] = { GL_R8, GL_RG8, GL_RGB8, GL_RGBA8 };
	const GLuint formats[] = { GL_RED, GL_RG, GL_RGB, GL_RGBA };
	if(texture_handle) {
		glBindTexture(GL_TEXTURE_2D, texture_handle);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalformats[channels - 1], size_x, size_y);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size_x, size_y, formats[channels - 1], GL_UNSIGNED_BYTE, data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	return texture_handle;
}
GLuint make_gl_texture(simple_fs::directory const& dir, native_string_view file_name) {
	auto file = open_file(dir, file_name);
	if(!file)
		return 0;
	auto image = load_stb_image(*file);
	return make_gl_texture(image.data, image.size_x, image.size_y, image.channels);
}

void set_gltex_parameters(GLuint texture_handle, GLuint texture_type, GLuint filter, GLuint wrap) {
	glBindTexture(texture_type, texture_handle);
	glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, wrap);
	if(filter == GL_LINEAR_MIPMAP_NEAREST || filter == GL_LINEAR_MIPMAP_LINEAR) {
		glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenerateMipmap(texture_type);
	} else {
		glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	glBindTexture(texture_type, 0);
}
void set_gltex_parameters(GLuint texture_handle, GLuint texture_type, GLuint filter, GLuint wrap_a, GLuint wrap_b) {
	glBindTexture(texture_type, texture_handle);
	glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, wrap_a);
	glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, wrap_b);
	if(filter == GL_LINEAR_MIPMAP_NEAREST || filter == GL_LINEAR_MIPMAP_LINEAR) {
		glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenerateMipmap(texture_type);
	} else {
		glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	glBindTexture(texture_type, 0);
}

GLuint load_texture_array_from_file(simple_fs::file& file, int32_t tiles_x, int32_t tiles_y) {
	auto image = load_stb_image(file);

	GLuint texture_handle = 0;
	glGenTextures(1, &texture_handle);
	if(texture_handle) {
		glBindTexture(GL_TEXTURE_2D_ARRAY, texture_handle);

		size_t p_dx = image.size_x / tiles_x; // Pixels of each tile in x
		size_t p_dy = image.size_y / tiles_y; // Pixels of each tile in y
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, GLsizei(p_dx), GLsizei(p_dy), GLsizei(tiles_x * tiles_y), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, image.size_x);
		glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, image.size_y);

		for(int32_t x = 0; x < tiles_x; x++)
			for(int32_t y = 0; y < tiles_y; y++)
				glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, GLint(x * tiles_x + y), GLsizei(p_dx), GLsizei(p_dy), 1, GL_RGBA, GL_UNSIGNED_BYTE, ((uint32_t const*)image.data) + (x * p_dy * image.size_x + y * p_dx));

		set_gltex_parameters(texture_handle, GL_TEXTURE_2D_ARRAY, GL_LINEAR_MIPMAP_NEAREST, GL_REPEAT);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
	}
	return texture_handle;
}

void render_subrect(ogl::data const& state, float target_x, float target_y, float target_width, float target_height, float source_x, float source_y, float source_width, float source_height, GLuint texture_handle) {
	bind_vertices_by_rotation(state, ui::rotation::upright, false, false);
	GLuint subroutines[2] = { parameters::enabled, parameters::subsprite_c };
	glUniform2ui(state.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_handle);

	glUniform4f(state.ui_shader_d_rect_uniform, target_x, target_y, target_width, target_height);
	glUniform4f(state.ui_shader_subrect_uniform, source_x /* x offset */, source_width /* x width */, source_y /* y offset */, source_height /* y height */);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

bezier_path::~bezier_path() {
	if(data_vao) {
		glDeleteTextures(1, &dat_texture);
		glDeleteBuffers(1, &data_vbo);
		glDeleteBuffers(1, &dat_buffer);
		glDeleteVertexArrays(1, &data_vao);
		data_vbo = 0;
		data_vao = 0;
		dat_texture = 0;
		dat_buffer = 0;
	}
}
void bezier_path::update_vbo() {
	if(!data_vao) {
		glGenVertexArrays(1, &data_vao);
		glGenBuffers(1, &data_vbo);

		glBindVertexArray(data_vao);

		glBindBuffer(GL_ARRAY_BUFFER, data_vbo);

		glBindVertexBuffer(0, data_vbo, 0, sizeof(bezier_vertex));

		glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, offsetof(bezier_vertex, base_point_0));
		glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, offsetof(bezier_vertex, base_point_1));
		glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, offsetof(bezier_vertex, control_point_0));
		glVertexAttribFormat(3, 2, GL_FLOAT, GL_FALSE, offsetof(bezier_vertex, control_point_1));
		glVertexAttribFormat(4, 1, GL_FLOAT, GL_FALSE, offsetof(bezier_vertex, length_offset));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(4);
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(1, 0);
		glVertexAttribBinding(2, 0);
		glVertexAttribBinding(3, 0);
		glVertexAttribBinding(4, 0);

		glGenTextures(1, &dat_texture);
		glGenBuffers(1, &dat_buffer);

		glBindBuffer(GL_TEXTURE_BUFFER, dat_buffer);

		glBindTexture(GL_TEXTURE_BUFFER, dat_texture);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, dat_buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);
	}

	glBindVertexArray(data_vao);
	glBindBuffer(GL_ARRAY_BUFFER, data_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bezier_vertex) * path_data.size(), path_data.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_TEXTURE_BUFFER, dat_buffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(extra_data_s) * extra_data.size(), extra_data.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}
void bezier_path::render() {
	glPatchParameteri(GL_PATCH_VERTICES, 1);
	glBindVertexArray(data_vao);
	glBindBuffer(GL_ARRAY_BUFFER, data_vbo);
	glDrawArrays(GL_PATCHES, 0, (GLsizei)path_data.size());
}
} // namespace ogl
