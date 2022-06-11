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

#include <fstream>
#include <nlohmann/json.hpp>

#include "battle_parser.hh"
#include "mcts_manager.hh"
#include "random_player.hh"

int
main()
{
	/*
	 * auto battle_parser = pokezero::BattleParser();
	 *
	 * nlohmann::json state;
	 * std::ifstream ifstate("pokemon-showdown/battle_test_jsons/test.json");
	 * ifstate >> state;
	 *
	 * battle_parser.setState(state.dump());
	 * battle_parser.getMLVec();
	 */

	auto manager = pokezero::Manager<showdown::RandomPlayer, showdown::RandomPlayer>("manager");
	manager.start();
}
