#include "window.hpp"
#include "opengl_wrapper.hpp"
#include "resource.h"
#include "user_interactions.hpp"

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "Windows.h"
#include "Windowsx.h"
#include "GL/wglew.h"
#include "sound.hpp"
#include <Shlobj.h>
#include <usp10.h>
#include <initguid.h>
#include <inputscope.h>
#include <Textstor.h>
#include <tsattrs.h>
#include <msctf.h>
#include <olectl.h>

#define WM_GRAPHNOTIFY (WM_APP + 1)

namespace window {

bool is_key_depressed(sys::virtual_key key) {
	return GetKeyState(int32_t(key)) & 0x8000;
}

sys::key_modifiers get_current_modifiers() {
	uint32_t val =
			uint32_t((GetKeyState(VK_CONTROL) & 0x8000) ? sys::key_modifiers::modifiers_ctrl : sys::key_modifiers::modifiers_none) |
			uint32_t((GetKeyState(VK_MENU) & 0x8000) ? sys::key_modifiers::modifiers_alt : sys::key_modifiers::modifiers_none) |
			uint32_t((GetKeyState(VK_SHIFT) & 0x8000) ? sys::key_modifiers::modifiers_shift : sys::key_modifiers::modifiers_none);
	return sys::key_modifiers(val);
}

static HCURSOR cursors[9] = { HCURSOR(NULL) };
static int cursor_size = 0;

void change_cursor(HWND hwnd, simple_fs::file_system common_fs, cursor_type type) {
	auto root = simple_fs::get_root(common_fs);
	auto gfx_dir = simple_fs::open_directory(root, NATIVE("gfx"));
	auto cursors_dir = simple_fs::open_directory(gfx_dir, NATIVE("cursors"));

	if(cursors[uint8_t(type)] == HCURSOR(NULL)) {
		native_string_view fname = NATIVE("normal.cur");
		switch(type) {
		case cursor_type::normal:
			fname = NATIVE("normal.cur");
			break;
		case cursor_type::busy:
			fname = NATIVE("busy.ani");
			break;
		case cursor_type::drag_select:
			fname = NATIVE("dragselect.ani");
			break;
		case cursor_type::hostile_move:
			fname = NATIVE("attack_move.ani");
			break;
		case cursor_type::friendly_move:
			fname = NATIVE("friendly_move.ani");
			break;
		case cursor_type::no_move:
			fname = NATIVE("no_move.ani");
			break;
		case cursor_type::text:
			cursors[uint8_t(type)] = LoadCursor(NULL, IDC_IBEAM);
			SetCursor(cursors[uint8_t(type)]);
			SetClassLongPtr(hwnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(cursors[uint8_t(type)]));
			return;
		default:
			fname = NATIVE("normal.cur");
			break;
		}

		// adapted from https://stackoverflow.com/questions/34065/how-to-read-a-value-from-the-windows-registry

		HKEY hKey;
		auto response = RegOpenKeyExW(HKEY_CURRENT_USER, NATIVE("Software\\Microsoft\\Accessibility"), 0, KEY_READ, &hKey);
		bool exists = (response == ERROR_SUCCESS);
		auto cursor_size_key = NATIVE("CursorSize");

		uint32_t cursor_size = 1;

		if(exists) {
			DWORD dwBufferSize(sizeof(DWORD));
			DWORD nResult(0);
			LONG get_cursor_size_error = RegQueryValueExW(hKey,
				cursor_size_key,
				0,
				NULL,
				reinterpret_cast<LPBYTE>(&nResult),
				&dwBufferSize);
			if(get_cursor_size_error == ERROR_SUCCESS) {
				cursor_size = nResult;
			}
		}

		RegCloseKey(hKey);

		if(auto f = simple_fs::peek_file(cursors_dir, fname); f) {
			auto path = simple_fs::get_full_name(*f);
			cursors[uint8_t(type)] = (HCURSOR)LoadImageW(nullptr, path.c_str(), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE); //.cur or .ani

			if(cursors[uint8_t(type)] == HCURSOR(NULL)) {
				cursors[uint8_t(type)] = LoadCursor(nullptr, IDC_ARROW); //default system cursor
				cursor_size = GetSystemMetrics(SM_CXCURSOR) * cursor_size / 2;
			} else {
				cursor_size = GetSystemMetrics(SM_CXCURSOR) * cursor_size / 2;
			}
		} else {
			cursors[uint8_t(type)] = LoadCursor(nullptr, IDC_ARROW); //default system cursor
			cursor_size = GetSystemMetrics(SM_CXCURSOR) * cursor_size / 2;
		}
	}
	SetCursor(cursors[uint8_t(type)]);
	SetClassLongPtr(hwnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(cursors[uint8_t(type)]));
}

int32_t cursor_blink_ms() {
	static int32_t ms = []() {auto t = GetCaretBlinkTime(); return t == INFINITE ? 0 : t * 2; }();
	return ms;
}
int32_t double_click_ms() {
	static int32_t ms = GetDoubleClickTime();
	return ms;
}

void emit_error_message(std::string const& content, bool fatal) {
	static const char* msg1 = "The program has encountered a fatal error";
	static const char* msg2 = "The program has encountered the following problems";
	MessageBoxA(nullptr, content.c_str(), fatal ? msg1 : msg2, MB_OK | (fatal ? MB_ICONERROR : MB_ICONWARNING));
	if(fatal) {
		std::exit(EXIT_FAILURE);
	}
}

enum class lock_state : uint8_t {
	unlocked, locked_read, locked_readwrite
};

struct mouse_sink {
	ITfMouseSink* sink;
	LONG range_start;
	LONG range_length;
};

template<class Interface>
inline void safe_release(Interface*& i) {
	if(i) {
		i->Release();
		i = nullptr;
	}
}
} // namespace window
