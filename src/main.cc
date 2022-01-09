/*
 * PokeZero: ML Pokemon battling bot
 * Copyright (C) 2022  Mingu Kim & David Hughes
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "main.hh"

#include "parse_state.hh"
#include "player.hh"
#include "random_player.hh"
#include "showdown.hh"

int
main()
{ // Load example state as json
	parse_state::json state;
	std::ifstream ifstate("pokemon-showdown/battle_test_jsons/test.json");
	ifstate >> state;

	// Convert it to a string
	std::string state_string = state.dump();

	// Load relevant reference data json files
	parse_state::AllPokemonData all_data = parse_state::generateAllData();

	// Parse state string
	parse_state::parseState(state_string, all_data);
}
