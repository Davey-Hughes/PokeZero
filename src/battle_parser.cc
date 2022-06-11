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

#include "battle_parser.hh"

#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

#include "debug_helper.hh"

namespace pokezero {
BattleParser::BattleParser()
{
	this->dex_data = this->readPokedexData();
}

/*
 * Loads in required pokedex data nlohmann::json files
 */
PokedexData
BattleParser::readPokedexData()
{
	PokedexData data;

	// TODO: handle paths better?
	std::ifstream ifmoves("data/move_id.json");
	std::ifstream ifpokedex("data/pokedex.json");
	std::ifstream ifabilities("data/ability_id.json");
	std::ifstream ifitems("data/item_id.json");
	std::ifstream iftypes("data/type_id.json");
	std::ifstream ifstatus("data/status.json");
	std::ifstream ifvolatiles("data/volatiles.json");
	std::ifstream ifsideconds("data/side_conds.json");
	std::ifstream ifslotconds("data/slot_conds.json");
	std::ifstream ifterrain("data/terrain.json");
	std::ifstream ifweather("data/weather.json");

	ifmoves >> data.all_moves;
	ifpokedex >> data.pokedex;
	ifabilities >> data.abilities;
	ifitems >> data.items;
	iftypes >> data.all_types;
	ifstatus >> data.all_status;
	ifvolatiles >> data.all_volatiles;
	ifsideconds >> data.side_conds;
	ifslotconds >> data.slot_conds;
	ifterrain >> data.all_terrain;
	ifweather >> data.all_weather;

	return data;
}

/*
 * Print each field of BattleState
 *
 * TODO: finish implementation
 */
std::string
BattleParser::stringifyBattleState(BattleState &state)
{
	std::stringstream output;

	std::string indent = "    ";
	std::string double_indent = "        ";
	for (size_t i = 0; i < 2; i++) {
		if (i == 0) {
			output << "Side: Self" << std::endl;
		} else {
			output << "Side: Opponent" << std::endl;
		}
		for (size_t j = 1; j < 7; j++) {
			output << indent << "Pokemon " << j << ":" << std::endl;
			output << double_indent << "isActive: " << state.sides[i].pokemon[j].active << std::endl;
		}
	}

	return output.str();
}

/*
 * handle the response from the accompanying node process
 *
 * returns false if the battleState was null, true otherwise
 */
BattleParser::ResponseType
BattleParser::handleResponse(const std::string &response)
{
	auto res_json = nlohmann::json::parse(response);

	if (res_json["type"] == nullptr) {
		return BattleParser::EMPTY;
	} else if (res_json["type"] == "end") {
		return BattleParser::END;
	}

	addState(res_json);

	if (res_json["winner"] != nullptr) {
		this->winner = res_json["winner"];
	}

	return BATTLESTATE;
}

/*
 * add a state to the states vector
 *
 * TODO: prevent extra adds
 */
void
BattleParser::addState(nlohmann::json state)
{
	this->turns.insert(this->turns.end(), state);
}

std::string
BattleParser::getStateStr(int turn_num)
{
	return this->turns.at(turn_num)["battleState"].dump();
}
int
BattleParser::getStateId(int turn_num)
{
	return this->turns.at(turn_num)["id"];
}

/*
 * Parses nlohmann::json string of battle state and returns array of extracted important
 * variables, as outlined in comments for BattleState struct.
 */
MLVec
BattleParser::getMLVec(int turn_num)
{
	nlohmann::json state = this->turns.at(turn_num)["battleState"];

	std::array<double, BattleStateSize> state_arr;
	state_arr.fill(-1);
	BattleState *pokestate = (BattleState *) state_arr.data();

	if (this->debug) {
		std::cout << "sizeof BattleState: " << sizeof(BattleState) << '\n';
		std::cout << "sizeof double: " << sizeof(double) << '\n';
		std::cout << "BattleState num elements: " << BattleStateSize << '\n';
	}

	for (size_t side_i = 0; side_i < 2; side_i++) {
		// Iterate through map of Pokemon
		std::map<std::string, nlohmann::json> poke_map;
		for (nlohmann::json &mon: state["sides"][side_i]["pokemon"]) {
			std::string poke_id = mon["speciesState"]["id"];
			poke_map.insert(std::make_pair(poke_id, mon));
		}

		int poke_idx = 0;
		for (auto &[_, pokemon_curr]: poke_map) {
			auto poke = &(pokestate->sides[side_i].pokemon[poke_idx]);

			// Append their active status
			poke->active = 2 * int(pokemon_curr["isActive"]) - 1;

			// Append Pokemon ability
			std::string ability = pokemon_curr["ability"];
			poke->ability = this->dex_data.abilities[ability];

			// Append Pokemon item
			std::string item = pokemon_curr["item"];
			poke->item = this->dex_data.items[item];

			// Append Pokemon types
			for (std::string mon_type: pokemon_curr["types"]) {
				if (this->dex_data.all_types.contains(mon_type)) {
					int type_id = this->dex_data.all_types[mon_type];
					poke->types[type_id] = 1;
				}
			}

			// Append move_id, move pp, and move status
			std::map<int, nlohmann::json> move_map;
			for (nlohmann::json &move: pokemon_curr["moveSlots"]) {
				std::string move_id = move["id"];
				move_map.insert(std::make_pair(this->dex_data.all_moves[move_id], move));
			}

			int move_idx = 0;
			for (auto &[move_id, move_json]: move_map) {
				poke->moves[move_idx].id = move_id;
				poke->moves[move_idx].pp =
					2 * double((move_json)["pp"]) / double((move_json)["maxpp"]) - 1;
				poke->moves[move_idx].disabled = 2 * int((move_json)["disabled"]) - 1;
				move_idx++;
			}

			// Append status
			std::string curr_status = pokemon_curr["status"];
			poke->status[this->dex_data.all_status[curr_status]] = 1;

			// Append stats
			poke->stats[0] =
				2 * double(pokemon_curr["hp"]) / double(pokemon_curr["baseStoredStats"]["hp"]) - 1;
			poke->stats[1] = this->normStat(pokemon_curr["baseStoredStats"]["hp"]);
			poke->stats[2] = this->normStat(pokemon_curr["baseStoredStats"]["atk"]);
			poke->stats[3] = this->normStat(pokemon_curr["baseStoredStats"]["def"]);
			poke->stats[4] = this->normStat(pokemon_curr["baseStoredStats"]["spa"]);
			poke->stats[5] = this->normStat(pokemon_curr["baseStoredStats"]["spd"]);
			poke->stats[6] = this->normStat(pokemon_curr["baseStoredStats"]["spe"]);

			// Append stat boosts
			poke->boosts[0] = this->normBoost(pokemon_curr["boosts"]["atk"]);
			poke->boosts[1] = this->normBoost(pokemon_curr["boosts"]["def"]);
			poke->boosts[2] = this->normBoost(pokemon_curr["boosts"]["spa"]);
			poke->boosts[3] = this->normBoost(pokemon_curr["boosts"]["spd"]);
			poke->boosts[4] = this->normBoost(pokemon_curr["boosts"]["spe"]);
			poke->boosts[5] = this->normBoost(pokemon_curr["boosts"]["accuracy"]);
			poke->boosts[6] = this->normBoost(pokemon_curr["boosts"]["evasion"]);

			// Append trapped
			poke->trapped = 2 * int(pokemon_curr["trapped"]) - 1;

			// Append Pokemon volatiles if active
			if (pokemon_curr["isActive"]) {
				if (pokemon_curr["volatiles"].size() > 0) {
					for (nlohmann::json &vol: pokemon_curr["volatiles"]) {
						if (this->dex_data.all_volatiles.contains(vol["id"])) {
							std::string vol_id = vol["id"];
							pokestate->sides[side_i]
								.volatiles[this->dex_data.all_volatiles[vol_id]] = 1;
						}
					}
				}
			}
			poke_idx++;
		}

		// clang-format off
		// Append side conditions
		if (state["sides"][side_i]["sideConditions"].size() > 0) {
			for (nlohmann::json &cond: state["sides"][side_i]["sideConditions"]) {
				if (this->dex_data.side_conds.contains(cond["id"])) {
					std::string cond_id = cond["id"];
					if (cond_id == "stealthrock") {
						pokestate->sides[side_i].stealthrock = 1;
					} else if (cond_id == "stickyweb") {
						pokestate->sides[side_i].stickyweb = 1;
					} else if (cond_id == "spikes") {
						pokestate->sides[side_i].spikes_ctr = double(this->dex_data.side_conds["spikes"]["layers"]) / 3 * 2 - 1;
					} else if (cond_id == "toxicspikes") {
						pokestate->sides[side_i].tspikes_ctr = double(this->dex_data.side_conds["toxicspikes"]["layers"]) - 1;
					} else if (cond_id == "reflect") {
						pokestate->sides[side_i].ref_ctr = double(this->dex_data.side_conds["reflect"]["duration"]) / 7 * 2 - 1;
					} else if (cond_id == "lightscreen") {
						pokestate->sides[side_i].ls_ctr = double(this->dex_data.side_conds["lightscreen"]["duration"]) / 7 * 2 - 1;
					} else if (cond_id == "auroraveil") {
						pokestate->sides[side_i].av_ctr = double(this->dex_data.side_conds["auroraveil"]["duration"]) / 7 * 2 - 1;
					} else if (cond_id == "tailwind") {
						pokestate->sides[side_i].tw_ctr = double(this->dex_data.side_conds["tailwind"]["duration"]) / 7 * 2 - 1;
					}
				}
			}
		}
		// clang-format on

		// Append slot conditions (future sight, wish)
		if (state["sides"][side_i]["slotConditions"].size() > 0) {
			for (nlohmann::json &cond: state["sides"][side_i]["slotConditions"]) {
				if (this->dex_data.slot_conds.contains(cond["id"])) {
					std::string cond_id = cond["id"];
					if (cond_id == "wish") {
						pokestate->sides[side_i].wish_ctr =
							double(this->dex_data.slot_conds["wish"]["duration"]) * 2 - 1;
						pokestate->sides[side_i].wish_hp =
							this->normStat(double(this->dex_data.slot_conds["wish"]["hp"]));
					} else if (cond_id == "futuremove") {
						pokestate->sides[side_i].future_move = 1;
						pokestate->sides[side_i].future_ctr =
							double(this->dex_data.slot_conds["futuremove"]["duration"]) - 1;
					}
				}
			}
		}
	}

	// Append weather
	if (state["field"]["weatherState"].size() > 1) {
		std::string w_id = state["field"]["weatherState"]["id"];
		pokestate->weather[this->dex_data.all_weather[w_id]] = 1;
		pokestate->weather_ctr = double(state["field"]["weatherState"]["duration"]) / 7 * 2 - 1;
	}

	// Append terrain
	if (state["field"]["terrainState"].size() > 1) {
		std::string t_id = state["field"]["terrainState"]["id"];
		pokestate->terrain[this->dex_data.all_terrain[t_id]] = 1;
		pokestate->terrain_ctr = double(state["field"]["terrainState"]["duration"]) / 7 * 2 - 1;
	}

	// Append trick room
	if (state["field"]["pseudoWeather"].size() > 0) {
		for (nlohmann::json pweather: state["field"]["pseudoWeather"]) {
			if (pweather["id"] == "trickroom") {
				pokestate->trick_room = 1;
				pokestate->tr_ctr = pweather["duration"];
			}
		}
	}

	if (this->debug) {
		// Print out state vector
		std::cout << "{\n";
		size_t count = 0;
		for (double &i: state_arr) {
			std::cout.precision(5);
			std::cout << std::setfill(' ') << std::setw(10) << i << " ";
			count++;
			if (count % 10 == 0) {
				std::cout << '\n';
			}
		}
		std::cout << "\n}\n";
		std::cout << this->stringifyBattleState(*pokestate);
	}

	return state_arr;
}
} // namespace pokezero
