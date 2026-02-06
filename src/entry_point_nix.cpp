#include "system_state.hpp"

static sys::state game_state;

int main(int argc, char* argv[]) {
	add_root(game_state.common_fs, NATIVE("."));


	//No args provided.
	if(argc <= 1) {

	} else {

	}

	game_state.load_user_settings();


		std::thread update_thread([&]() { game_state.game_loop(); });

		window::create_window(game_state, window::creation_parameters{ 1024, 780, window::window_state::maximized, game_state.user_settings.prefer_fullscreen });
		game_state.quit_signaled.store(true, std::memory_order_release);

		update_thread.join();

	return EXIT_SUCCESS;
}

