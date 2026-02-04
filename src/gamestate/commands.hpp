#pragma once
#include "data_ids.hpp"
#include "common_types.hpp"
#include "constants_dcon.hpp"
#include "constants.hpp"
#include "container_types.hpp"
#include "commands_containers.hpp"
namespace command {

enum class command_type : uint8_t {
	invalid = 0,

};


struct command_type_data {
	uint32_t min_payload_size;
	uint32_t max_payload_size;
	command_type_data(uint32_t _min_payload_size, uint32_t _max_payload_size) {
		min_payload_size = _min_payload_size;
		max_payload_size = _max_payload_size;
	}
};


static ankerl::unordered_dense::map<command::command_type, command::command_type_data> command_type_handlers = {
	//{command_type::change_nat_focus, command_type_data{ sizeof(command::national_focus_data), sizeof(command::national_focus_data) } },
};

// returns true if the command was performed, false if not
bool execute_command(sys::state& state, command_data& c);
void execute_pending_commands(sys::state& state);
bool can_perform_command(sys::state& state, command_data& c);


} // namespace command

