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

#ifndef PARSE_STATE_HH
#define PARSE_STATE_HH

#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <nlohmann/json.hpp>

#include "debug_helper.hh"

namespace parse_state {

struct Move {
	double id;
	double pp;
	double disabled;
};

struct Pokemon {
	double active;
	double types[18];
	double ability;
	double item;
	double status[8];
	double stats[7];
	double boosts[7];
	double trapped;
	Move moves[4];
};

struct Side {
	Pokemon pokemon[6];
	double volatiles[15];
	double stealthrock;
	double stickyweb;
	double spikes_ctr;
	double tspikes_ctr;
	double ref_ctr;
	double ls_ctr;
	double av_ctr;
	double tw_ctr;
	double wish_ctr;
	double wish_hp;
	double future_move;
	double future_ctr;
};

struct PokemonState {
	Side sides[2];
	double weather[4];
	double weather_ctr;
	double terrain[4];
	double terrain_ctr;
	double trick_room;
	double tr_ctr;
};

using json = nlohmann::json;

double
normStat(double stat)
{
	return stat / 500 * 2 - 1;
}

double
normBoost(double boost)
{
	return (boost + 6) / 6 - 1;
}

constexpr size_t struct_arr_sizeof = sizeof(PokemonState) / sizeof(double);

std::array<double, struct_arr_sizeof>
parseState()
{
	// return torch.Tensor(ability+item+types+moves+move_pp + move_disabled +status+stats
	// +stat_boosts+sub_hp+trapped_counter+volatile_status)
	std::ifstream ifstate("pokemon-showdown/battle_test_jsons/test.json");
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
	json state, all_moves, pokedex, abilities, items, all_types, all_status, all_volatiles, sideconds, slotconds,
		all_terrain, all_weather;
	ifstate >> state;

	DEBUG_STDOUT("MADE IT HERE");

	ifmoves >> all_moves;
	ifpokedex >> pokedex;
	ifabilities >> abilities;
	ifitems >> items;
	iftypes >> all_types;
	ifstatus >> all_status;
	ifvolatiles >> all_volatiles;
	ifsideconds >> sideconds;
	ifslotconds >> slotconds;
	ifterrain >> all_terrain;
	ifweather >> all_weather;

	std::array<double, struct_arr_sizeof> state_arr;
	state_arr.fill(-1);
	PokemonState *pokestate = (PokemonState *) state_arr.data();

#ifdef DEBUG
	std::cout << "sizeof PokemonState: " << sizeof(PokemonState) << '\n';
	std::cout << "sizeof double: " << sizeof(double) << '\n';
	std::cout << "PokemonState num elements: " << struct_arr_sizeof << '\n';
	// /* set some values via the struct */
	// pokestate->a = 1;
	// mystruct->b = 5;
	// mystruct->substruct_bb.l = -5;
	// mystruct->myarray_in_struct[4] = 20;

#endif // DEBUG

	for (int side_i = 0; side_i < 2; side_i++) {
		// Iterate through map of Pokemon
		std::map<std::string, json> poke_map;
		for (json &mon: state["sides"][side_i]["pokemon"]) {
			std::string poke_id = mon["speciesState"]["id"];
			poke_map.insert(std::make_pair(poke_id, mon));
		}

		std::map<std::string, json>::iterator poke_it = poke_map.begin();
		int poke_idx = 0;
		while (poke_it != poke_map.end()) {
			json pokemon_curr = poke_it->second;

			// auto poke = &(pokestate->sides[side_i].pokemon[poke_idx]);

			// Append their active status
			pokestate->sides[side_i].pokemon[poke_idx].active = 2 * int(pokemon_curr["isActive"]) - 1;

			// Append Pokemon ability
			std::string ability = pokemon_curr["ability"];
			pokestate->sides[side_i].pokemon[poke_idx].ability = abilities[ability];

			// Append Pokemon item
			std::string item = pokemon_curr["item"];
			pokestate->sides[side_i].pokemon[poke_idx].item = items[item];

			// Append Pokemon types
			for (std::string mon_type: pokemon_curr["types"]) {
				if (all_types.contains(mon_type)) {
					int type_id = all_types[mon_type];
					pokestate->sides[side_i].pokemon[poke_idx].types[type_id] = 1;
				}
			}

			// Append move_id, move pp, and move status
			std::map<int, json> move_map;
			for (json &move: pokemon_curr["moveSlots"]) {
				std::string move_id = move["id"];
				move_map.insert(std::make_pair(all_moves[move_id], move));
			}
			std::map<int, json>::iterator move_it = move_map.begin();
			int move_idx = 0;
			while (move_it != move_map.end()) {
				pokestate->sides[side_i].pokemon[poke_idx].moves[move_idx].id = move_it->first;
				pokestate->sides[side_i].pokemon[poke_idx].moves[move_idx].pp =
					2 * double((move_it->second)["pp"]) / double((move_it->second)["maxpp"]) - 1;
				pokestate->sides[side_i].pokemon[poke_idx].moves[move_idx].disabled =
					2 * int((move_it->second)["disabled"]) - 1;
				move_it++;
				move_idx++;
			}

			// Append status
			std::string curr_status = pokemon_curr["status"];
			pokestate->sides[side_i].pokemon[poke_idx].status[all_status[curr_status]] = 1;

			// Append stats
			pokestate->sides[side_i].pokemon[poke_idx].stats[0] =
				2 * double(pokemon_curr["hp"]) / double(pokemon_curr["baseStoredStats"]["hp"]) - 1;
			pokestate->sides[side_i].pokemon[poke_idx].stats[1] =
				normStat(pokemon_curr["baseStoredStats"]["hp"]);
			pokestate->sides[side_i].pokemon[poke_idx].stats[2] =
				normStat(pokemon_curr["baseStoredStats"]["atk"]);
			pokestate->sides[side_i].pokemon[poke_idx].stats[3] =
				normStat(pokemon_curr["baseStoredStats"]["def"]);
			pokestate->sides[side_i].pokemon[poke_idx].stats[4] =
				normStat(pokemon_curr["baseStoredStats"]["spa"]);
			pokestate->sides[side_i].pokemon[poke_idx].stats[5] =
				normStat(pokemon_curr["baseStoredStats"]["spd"]);
			pokestate->sides[side_i].pokemon[poke_idx].stats[6] =
				normStat(pokemon_curr["baseStoredStats"]["spe"]);

			// Append stat boosts
			pokestate->sides[side_i].pokemon[poke_idx].boosts[0] = normBoost(pokemon_curr["boosts"]["atk"]);
			pokestate->sides[side_i].pokemon[poke_idx].boosts[1] = normBoost(pokemon_curr["boosts"]["def"]);
			pokestate->sides[side_i].pokemon[poke_idx].boosts[2] = normBoost(pokemon_curr["boosts"]["spa"]);
			pokestate->sides[side_i].pokemon[poke_idx].boosts[3] = normBoost(pokemon_curr["boosts"]["spd"]);
			pokestate->sides[side_i].pokemon[poke_idx].boosts[4] = normBoost(pokemon_curr["boosts"]["spe"]);
			pokestate->sides[side_i].pokemon[poke_idx].boosts[5] =
				normBoost(pokemon_curr["boosts"]["accuracy"]);
			pokestate->sides[side_i].pokemon[poke_idx].boosts[6] =
				normBoost(pokemon_curr["boosts"]["evasion"]);

			// Append trapped
			pokestate->sides[side_i].pokemon[poke_idx].trapped = 2 * int(pokemon_curr["trapped"]) - 1;

			// Append Pokemon volatiles if active
			if (pokemon_curr["isActive"]) {
				if (pokemon_curr["volatiles"].size() > 0) {
					for (json vol: pokemon_curr["volatiles"]) {
						if (all_volatiles.contains(vol["id"])) {
							std::string vol_id = vol["id"];
							pokestate->sides[side_i].volatiles[all_volatiles[vol_id]] = 1;
						}
					}
				}
			}
			poke_it++;
			poke_idx++;
		}

		// Append side conditions
		if (state["sides"][side_i]["sideConditions"].size() > 0) {
			for (json &cond: state["sides"][side_i]["sideConditions"]) {
				if (sideconds.contains(cond["id"])) {
					std::string cond_id = cond["id"];
					if (cond_id == "stealthrock") {
						pokestate->sides[side_i].stealthrock = 1;
					} else if (cond_id == "stickyweb") {
						pokestate->sides[side_i].stickyweb = 1;
					} else if (cond_id == "spikes") {
						pokestate->sides[side_i].spikes_ctr =
							double(sideconds["spikes"]["layers"]) / 3 * 2 - 1;
					} else if (cond_id == "toxicspikes") {
						pokestate->sides[side_i].tspikes_ctr =
							double(sideconds["toxicspikes"]["layers"]) - 1;
					} else if (cond_id == "reflect") {
						pokestate->sides[side_i].ref_ctr =
							double(sideconds["reflect"]["duration"]) / 7 * 2 - 1;
					} else if (cond_id == "lightscreen") {
						pokestate->sides[side_i].ls_ctr =
							double(sideconds["lightscreen"]["duration"]) / 7 * 2 - 1;
					} else if (cond_id == "auroraveil") {
						pokestate->sides[side_i].av_ctr =
							double(sideconds["auroraveil"]["duration"]) / 7 * 2 - 1;
					} else if (cond_id == "tailwind") {
						pokestate->sides[side_i].tw_ctr =
							double(sideconds["tailwind"]["duration"]) / 7 * 2 - 1;
					}
				}
			}
		}

		// Append slot conditions (future sight, wish)
		if (state["sides"][side_i]["slotConditions"].size() > 0) {
			for (json &cond: state["sides"][side_i]["slotConditions"]) {
				if (slotconds.contains(cond["id"])) {
					std::string cond_id = cond["id"];
					if (cond_id == "wish") {
						pokestate->sides[side_i].wish_ctr =
							double(slotconds["wish"]["duration"]) * 2 - 1;
						pokestate->sides[side_i].wish_hp =
							normStat(double(slotconds["wish"]["hp"]));
					} else if (cond_id == "futuremove") {
						pokestate->sides[side_i].future_move = 1;
						pokestate->sides[side_i].future_ctr =
							double(slotconds["futuremove"]["duration"]) - 1;
					}
				}
			}
		}
	}

	// Append weather
	if (state["field"]["weatherState"].size() > 1) {
		std::string w_id = state["field"]["weatherState"]["id"];
		pokestate->weather[all_weather[w_id]] = 1;
		pokestate->weather_ctr = double(state["field"]["weatherState"]["duration"]) / 7 * 2 - 1;
	}

	// Append terrain
	if (state["field"]["terrainState"].size() > 1) {
		std::string t_id = state["field"]["terrainState"]["id"];
		pokestate->terrain[all_terrain[t_id]] = 1;
		pokestate->terrain_ctr = double(state["field"]["terrainState"]["duration"]) / 7 * 2 - 1;
	}

	// Append trick room
	if (state["field"]["pseudoWeather"].size() > 0) {
		for (json pweather: state["field"]["pseudoWeather"]) {
			if (pweather["id"] == "trickroom") {
				pokestate->trick_room = 1;
				pokestate->tr_ctr = pweather["duration"];
			}
		}
	}

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
	return state_arr;
}
} // namespace parse_state

#endif /* PARSE_STATE_HH */
