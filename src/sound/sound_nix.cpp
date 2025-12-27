#include "sound.hpp"
#include "system_state.hpp"

#define MINIAUDIO_IMPLEMENTATION 1
#define MA_NO_FLAC 1
#define MA_NO_OPENSL 1
#define MA_NO_WEBAUDIO 1
#define MA_NO_SNDIO 1
#define MA_NO_AUDIO4 1
#define MA_NO_WINMM 1
#define MA_NO_WASAPI 1
#define MA_NO_WINMM 1
#define MA_NO_COREAUDIO 1
#include "miniaudio.h"

namespace sound {

sound_impl::sound_impl() {
	if(ma_engine_init(NULL, &engine) != MA_SUCCESS) {
		std::abort(); //TODO: This shouldn't be a cause for abort
	}
}

sound_impl::~sound_impl() {
	ma_engine_uninit(&engine);
}

void sound_impl::set_volume(std::optional<ma_sound>& sound, float volume) {
	if(sound.has_value()) {
		ma_sound_set_volume(&*sound, volume);
	}
}

void sound_impl::override_sound(std::optional<ma_sound>& sound, audio_instance& s, float volume) {
	if(sound.has_value()) {
		ma_sound_uninit(&*sound);
	}

	sound.reset();
	sound.emplace();
	ma_result result = ma_sound_init_from_file(&engine, s.filename.c_str(), 0, NULL, NULL, &*sound);
	if(result == MA_SUCCESS) {
		set_volume(sound, volume);
		ma_sound_start(&*sound);
	}
}

void sound_impl::play_music(int32_t track, float volume) {
	current_music = track;

	audio_instance audio{};
	audio.filename = music_list[track].filename.c_str();
	override_sound(music, audio, volume);
}

void sound_impl::play_new_track(sys::state& ws) {
	if(music_list.size() > 0) {
		int32_t result = int32_t(rand() % music_list.size()); // well aware that using rand is terrible, thanks
		for(uint32_t i = 0; i < 16 && result == last_music; i++) {
			result = int32_t(rand() % music_list.size());
		}
		play_music(result, ws.user_settings.master_volume * ws.user_settings.music_volume);
	}
}
void sound_impl::play_next_track(sys::state& ws) {
	if(music_list.size() > 0) {
		int32_t result = int32_t((last_music + 1) % music_list.size());
		play_music(result, ws.user_settings.master_volume * ws.user_settings.music_volume);
	}
}
void sound_impl::play_previous_track(sys::state& ws) {
	if(music_list.size() > 0) {
		int32_t result = int32_t((last_music - 1) % music_list.size());
		play_music(result, ws.user_settings.master_volume * ws.user_settings.music_volume);
	}
}

bool sound_impl::music_finished() {
	if(music.has_value())
		return music->atEnd;
	return true;
}

void initialize_sound_system(sys::state& state) {
	state.sound_ptr = std::make_unique<sound_impl>();

	auto root_dir = simple_fs::get_root(state.common_fs);
	auto const asset_dir = simple_fs::open_directory(root_dir, NATIVE("assets"));
	auto const music_dir = simple_fs::open_directory(asset_dir, NATIVE("music"));
	for(auto const& mp3_file : simple_fs::list_files(music_dir, NATIVE(".mp3"))) {
		audio_instance audio{};
		audio.set_file(simple_fs::get_full_name(mp3_file));
		state.sound_ptr->music_list.emplace_back(audio);
		auto file_name = simple_fs::get_full_name(mp3_file);
		if(parsers::native_has_fixed_suffix_ci(file_name.c_str(), file_name.c_str() + file_name.length(), NATIVE("thecoronation_titletheme.mp3")))
			state.sound_ptr->first_music = int32_t(state.sound_ptr->music_list.size()) - 1;
	}
	for(auto const& music_subdir : simple_fs::list_subdirectories(music_dir)) {
		for(auto const& mp3_file : simple_fs::list_files(music_subdir, NATIVE(".mp3"))) {
			audio_instance audio{};
			audio.set_file(simple_fs::get_full_name(mp3_file));
			state.sound_ptr->music_list.emplace_back(audio);
		}
	}

	struct {
		audio_instance* audio;
		native_string_view name;
	} vanilla_sound_table[] = {
		{ &state.sound_ptr->click_sound, NATIVE("click.wav") },
	};

	auto const sound_directory = simple_fs::open_directory(asset_dir, NATIVE("sfx"));
	for(const auto& e : vanilla_sound_table) {
		auto file_peek = simple_fs::peek_file(sound_directory, e.name);
		e.audio->set_file(file_peek ? simple_fs::get_full_name(*file_peek) : native_string());
	}
}
void change_effect_volume(sys::state& state, float v) {
	state.sound_ptr->set_volume(state.sound_ptr->effect_sound, v);
}
void change_interface_volume(sys::state& state, float v) {
	state.sound_ptr->set_volume(state.sound_ptr->interface_sound, v);
}
void change_music_volume(sys::state& state, float v) {
	state.sound_ptr->set_volume(state.sound_ptr->music, v);
}

void play_effect(sys::state& state, audio_instance& s, float volume) {
	if(state.sound_ptr->global_pause)
		return;
	state.sound_ptr->override_sound(state.sound_ptr->effect_sound, s, volume);
}
void play_interface_sound(sys::state& state, audio_instance& s, float volume) {
	if(state.sound_ptr->global_pause)
		return;
	state.sound_ptr->override_sound(state.sound_ptr->interface_sound, s, volume);
}

void stop_music(sys::state& state) {
	if(state.sound_ptr->music.has_value()) {
		ma_sound_stop(&*state.sound_ptr->music);
	}
}
void start_music(sys::state& state, float v) {
	if(v > 0.0f && state.sound_ptr->music_list.size() != 0) {
		if(state.sound_ptr->first_music != -1) {
			state.sound_ptr->play_music(state.sound_ptr->first_music, v);
		} else {
			if(state.sound_ptr->music.has_value()) {
				ma_sound_start(&*state.sound_ptr->music);
			}
		}
	}
}

void pause_all(sys::state& state) {
	if(state.sound_ptr.get()) {
		state.sound_ptr->global_pause = true;
		if(state.sound_ptr->effect_sound.has_value()) {
			ma_sound_stop(&*state.sound_ptr->effect_sound);
		}
		if(state.sound_ptr->interface_sound.has_value()) {
			ma_sound_stop(&*state.sound_ptr->interface_sound);
		}
		if(state.sound_ptr->music.has_value()) {
			ma_sound_stop(&*state.sound_ptr->music);
		}
	}
}
void resume_all(sys::state& state) {
	if(state.sound_ptr.get()) {
		state.sound_ptr->global_pause = false;
		if(state.sound_ptr->effect_sound.has_value()) {
			ma_sound_start(&*state.sound_ptr->effect_sound);
		}
		if(state.sound_ptr->interface_sound.has_value()) {
			ma_sound_start(&*state.sound_ptr->interface_sound);
		}
		if(state.sound_ptr->music.has_value()) {
			ma_sound_start(&*state.sound_ptr->music);
		}
	}
}

void update_music_track(sys::state& state) {
	if(state.sound_ptr->music_finished()) {
		state.sound_ptr->play_new_track(state);
	}
}

// returns the default click sound -- expect this list of functions to expand as
//    we implement more of the fixed sound effects
audio_instance& get_click_sound(sys::state& state) {
	return state.sound_ptr->click_sound;
}

void play_new_track(sys::state& state) {
	state.sound_ptr->play_new_track(state);
}
void play_next_track(sys::state& state) {
	state.sound_ptr->play_next_track(state);
}
void play_previous_track(sys::state& state) {
	state.sound_ptr->play_previous_track(state);
}

native_string get_current_track_name(sys::state& state) {
	if(state.sound_ptr->music.has_value())
		return state.sound_ptr->music_list[state.sound_ptr->current_music].filename;
	return "";
}

} // namespace sound
