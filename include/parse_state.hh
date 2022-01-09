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
	double id;       // id of move
	double pp;       // scaled ratio of current pp / max pp for move
	double disabled; // whether the move is currently disabled
};

struct Pokemon {
	double active;    // whether this Pokemon is currently active in battle
	double types[18]; // 1 if Pokemon has type, -1 otherwise
	double ability;   // id of ability
	double item;      // id of item
	double status[8]; // 1 if Pokemon has status condition, -1 otherwise
	double stats[7];  // scaled raw stats
	double boosts[7]; // stat boosts
	double trapped;   // 1 if Pokemon is trapped, -1 otherwise
	Move moves[4];    // moves Pokemon has
};

struct Side {
	Pokemon pokemon[6];   // party of 6 Pokemon
	double volatiles[15]; // 1 if Pokemon has volatile condition, -1 otherwise
	double stealthrock;   // if stealth rocks are up on given side
	double stickyweb;     // if sticky web is up on given side
	double spikes_ctr;    // number of spikes on given side
	double tspikes_ctr;   // number of toxic spikes on given side
	double ref_ctr;       // duration (in turns) remaining of reflect on given side
	double ls_ctr;        // duration (in turns) remaining of light screen on given side
	double av_ctr;        // duration (in turns) remaining of aurora veil on given side
	double tw_ctr;        // duration (in turns) remaining of tailwind on given side
	double wish_ctr;      // number of turns until wish heal on given side
	double wish_hp;       // amount of hp wish will heal
	double future_move;   // whether future sight has been casted on given side
	double future_ctr;    // how many turns until side's active will take damage from future sight
};

struct PokemonState {
	Side sides[2];      // both sides of Pokemon (self vs opponent)
	double weather[4];  // 1 if current weather, -1 otherwise
	double weather_ctr; // number of turns remaining of weather
	double terrain[4];  // 1 if terrain is up, -1 otherwise
	double terrain_ctr; // number of turns remaining of terrain
	double trick_room;  // 1 if trick room is up, -1 otherwise
	double tr_ctr;      // number of turns remaining of trick room
};

using json = nlohmann::json;

/*
 * Normalizes a Pokemon's stat to be approximately in range [-1, 1], assuming highest stat is 500
 */
double
normStat(double stat)
{
	return stat / 500 * 2 - 1;
}

/*
 * Normalizes stat boosts to [-1, 1]
 */
double
normBoost(double boost)
{
	return (boost + 6) / 6 - 1;
}

/*
 * TODO: Print each field of PokemonState (currently unfinished)
 */
void
printState(PokemonState *state)
{
	std::string indent = "    ";
	std::string double_indent = "        ";
	for (size_t i = 0; i < 2; i++) {
		if (i == 0) {
			std::cout << "Side: Self" << std::endl;
		} else {
			std::cout << "Side: Opponent" << std::endl;
		}
		for (size_t j = 1; j < 7; j++) {
			std::cout << indent << "Pokemon " << j << ":" << std::endl;
			std::cout << double_indent << "isActive: " << state->sides[i].pokemon[j].active << std::endl;
		}
	}
}

struct AllPokemonData {
	json all_moves;
	json pokedex;
	json abilities;
	json items;
	json all_types;
	json all_status;
	json all_volatiles;
	json side_conds;
	json slot_conds;
	json all_terrain;
	json all_weather;
};

/*
 * Loads in all data json files
 */
AllPokemonData
generateAllData()
{
	AllPokemonData data;
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

constexpr size_t struct_arr_sizeof = sizeof(PokemonState) / sizeof(double); // determine size of output state array

/*
 * Parses JSON string of battle state and returns array of extracted important variables,
 *
 * as outlined in comments for PokemonState struct.
 *
 */
std::array<double, struct_arr_sizeof>
parseState(const std::string &state_str, const AllPokemonData &all_data)
{
	json state = json::parse(state_str);
	std::array<double, struct_arr_sizeof> state_arr;
	state_arr.fill(-1);
	PokemonState *pokestate = (PokemonState *) state_arr.data();

#ifdef DEBUG
	std::cout << "sizeof PokemonState: " << sizeof(PokemonState) << '\n';
	std::cout << "sizeof double: " << sizeof(double) << '\n';
	std::cout << "PokemonState num elements: " << struct_arr_sizeof << '\n';
#endif // DEBUG

	for (size_t side_i = 0; side_i < 2; side_i++) {
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

			auto poke = &(pokestate->sides[side_i].pokemon[poke_idx]);

			// Append their active status
			poke->active = 2 * int(pokemon_curr["isActive"]) - 1;

			// Append Pokemon ability
			std::string ability = pokemon_curr["ability"];
			poke->ability = all_data.abilities[ability];

			// Append Pokemon item
			std::string item = pokemon_curr["item"];
			poke->item = all_data.items[item];

			// Append Pokemon types
			for (std::string mon_type: pokemon_curr["types"]) {
				if (all_data.all_types.contains(mon_type)) {
					int type_id = all_data.all_types[mon_type];
					poke->types[type_id] = 1;
				}
			}

			// Append move_id, move pp, and move status
			std::map<int, json> move_map;
			for (json &move: pokemon_curr["moveSlots"]) {
				std::string move_id = move["id"];
				move_map.insert(std::make_pair(all_data.all_moves[move_id], move));
			}
			std::map<int, json>::iterator move_it = move_map.begin();
			int move_idx = 0;
			while (move_it != move_map.end()) {
				poke->moves[move_idx].id = move_it->first;
				poke->moves[move_idx].pp =
					2 * double((move_it->second)["pp"]) / double((move_it->second)["maxpp"]) - 1;
				poke->moves[move_idx].disabled = 2 * int((move_it->second)["disabled"]) - 1;
				move_it++;
				move_idx++;
			}

			// Append status
			std::string curr_status = pokemon_curr["status"];
			poke->status[all_data.all_status[curr_status]] = 1;

			// Append stats
			poke->stats[0] =
				2 * double(pokemon_curr["hp"]) / double(pokemon_curr["baseStoredStats"]["hp"]) - 1;
			poke->stats[1] = normStat(pokemon_curr["baseStoredStats"]["hp"]);
			poke->stats[2] = normStat(pokemon_curr["baseStoredStats"]["atk"]);
			poke->stats[3] = normStat(pokemon_curr["baseStoredStats"]["def"]);
			poke->stats[4] = normStat(pokemon_curr["baseStoredStats"]["spa"]);
			poke->stats[5] = normStat(pokemon_curr["baseStoredStats"]["spd"]);
			poke->stats[6] = normStat(pokemon_curr["baseStoredStats"]["spe"]);

			// Append stat boosts
			poke->boosts[0] = normBoost(pokemon_curr["boosts"]["atk"]);
			poke->boosts[1] = normBoost(pokemon_curr["boosts"]["def"]);
			poke->boosts[2] = normBoost(pokemon_curr["boosts"]["spa"]);
			poke->boosts[3] = normBoost(pokemon_curr["boosts"]["spd"]);
			poke->boosts[4] = normBoost(pokemon_curr["boosts"]["spe"]);
			poke->boosts[5] = normBoost(pokemon_curr["boosts"]["accuracy"]);
			poke->boosts[6] = normBoost(pokemon_curr["boosts"]["evasion"]);

			// Append trapped
			poke->trapped = 2 * int(pokemon_curr["trapped"]) - 1;

			// Append Pokemon volatiles if active
			if (pokemon_curr["isActive"]) {
				if (pokemon_curr["volatiles"].size() > 0) {
					for (json &vol: pokemon_curr["volatiles"]) {
						if (all_data.all_volatiles.contains(vol["id"])) {
							std::string vol_id = vol["id"];
							pokestate->sides[side_i]
								.volatiles[all_data.all_volatiles[vol_id]] = 1;
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
				if (all_data.side_conds.contains(cond["id"])) {
					std::string cond_id = cond["id"];
					if (cond_id == "stealthrock") {
						pokestate->sides[side_i].stealthrock = 1;
					} else if (cond_id == "stickyweb") {
						pokestate->sides[side_i].stickyweb = 1;
					} else if (cond_id == "spikes") {
						pokestate->sides[side_i].spikes_ctr =
							double(all_data.side_conds["spikes"]["layers"]) / 3 * 2 - 1;
					} else if (cond_id == "toxicspikes") {
						pokestate->sides[side_i].tspikes_ctr =
							double(all_data.side_conds["toxicspikes"]["layers"]) - 1;
					} else if (cond_id == "reflect") {
						pokestate->sides[side_i].ref_ctr =
							double(all_data.side_conds["reflect"]["duration"]) / 7 * 2 - 1;
					} else if (cond_id == "lightscreen") {
						pokestate->sides[side_i].ls_ctr =
							double(all_data.side_conds["lightscreen"]["duration"]) / 7 * 2 -
							1;
					} else if (cond_id == "auroraveil") {
						pokestate->sides[side_i].av_ctr =
							double(all_data.side_conds["auroraveil"]["duration"]) / 7 * 2 -
							1;
					} else if (cond_id == "tailwind") {
						pokestate->sides[side_i].tw_ctr =
							double(all_data.side_conds["tailwind"]["duration"]) / 7 * 2 - 1;
					}
				}
			}
		}

		// Append slot conditions (future sight, wish)
		if (state["sides"][side_i]["slotConditions"].size() > 0) {
			for (json &cond: state["sides"][side_i]["slotConditions"]) {
				if (all_data.slot_conds.contains(cond["id"])) {
					std::string cond_id = cond["id"];
					if (cond_id == "wish") {
						pokestate->sides[side_i].wish_ctr =
							double(all_data.slot_conds["wish"]["duration"]) * 2 - 1;
						pokestate->sides[side_i].wish_hp =
							normStat(double(all_data.slot_conds["wish"]["hp"]));
					} else if (cond_id == "futuremove") {
						pokestate->sides[side_i].future_move = 1;
						pokestate->sides[side_i].future_ctr =
							double(all_data.slot_conds["futuremove"]["duration"]) - 1;
					}
				}
			}
		}
	}

	// Append weather
	if (state["field"]["weatherState"].size() > 1) {
		std::string w_id = state["field"]["weatherState"]["id"];
		pokestate->weather[all_data.all_weather[w_id]] = 1;
		pokestate->weather_ctr = double(state["field"]["weatherState"]["duration"]) / 7 * 2 - 1;
	}

	// Append terrain
	if (state["field"]["terrainState"].size() > 1) {
		std::string t_id = state["field"]["terrainState"]["id"];
		pokestate->terrain[all_data.all_terrain[t_id]] = 1;
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

#ifdef DEBUG
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
	printState(pokestate);
#endif // DEBUG

	return state_arr;
}
} // namespace parse_state

#endif /* PARSE_STATE_HH */
