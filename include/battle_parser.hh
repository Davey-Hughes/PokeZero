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

#ifndef BATTLE_PARSER_HH
#define BATTLE_PARSER_HH

#include <nlohmann/json.hpp>

#include "debug_helper.hh"

namespace {
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

struct BattleState {
	Side sides[2];      // both sides of Pokemon (self vs opponent)
	double weather[4];  // 1 if current weather, -1 otherwise
	double weather_ctr; // number of turns remaining of weather
	double terrain[4];  // 1 if terrain is up, -1 otherwise
	double terrain_ctr; // number of turns remaining of terrain
	double trick_room;  // 1 if trick room is up, -1 otherwise
	double tr_ctr;      // number of turns remaining of trick room
};

struct PokedexData {
	nlohmann::json all_moves;
	nlohmann::json pokedex;
	nlohmann::json abilities;
	nlohmann::json items;
	nlohmann::json all_types;
	nlohmann::json all_status;
	nlohmann::json all_volatiles;
	nlohmann::json side_conds;
	nlohmann::json slot_conds;
	nlohmann::json all_terrain;
	nlohmann::json all_weather;
};

// size of output state array
constexpr size_t BattleStateSize = sizeof(BattleState) / sizeof(double);
typedef std::array<double, BattleStateSize> MLVec;

} // namespace

namespace pokezero {
class BattleParser {
public:
	enum ResponseType { EMPTY, END, BATTLESTATE };

	// constructors
	BattleParser();

	// specify whether to print debugging statements
	BattleParser(bool debug) : BattleParser() { this->debug = debug; }

	void setState(const std::string &);
	void clearState();
	ResponseType handleResponse(const std::string &);
	int getStateId();
	std::string getStateStr() { return this->state.dump(); }

	MLVec getMLVec();

	std::string winner = ""; // winnner

private:
	bool debug = false;   // print debug info
	PokedexData dex_data; // struct with json info read from files

	int turn = -1;        // turn number
	int state_id = -1;    // state id generated from the node process
	nlohmann::json state; // current state

	PokedexData readPokedexData();

	/*
	 * Normalizes a Pokemon's stat to be approximately in range [-1, 1],
	 * assuming highest stat is 500
	 */
	double normStat(double stat) { return stat / 500 * 2 - 1; }

	/*
	 * Normalizes stat boosts to [-1, 1]
	 */
	double normBoost(double boost) { return (boost + 6) / 6 - 1; }

	std::string stringifyBattleState(BattleState &);
};
} // namespace pokezero

#endif /* BATTLE_PARSER_HH */
