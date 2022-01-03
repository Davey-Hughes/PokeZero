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

#include "showdown.hh"
#include "random_player.hh"
#include <fstream>
#include <main.hh>
#include <json.hpp>
#include <vector>
#include <map>
#include <iterator>

using json = nlohmann::json;

double normStat(double stat){
	return stat / 500 * 2 - 1;
}

double normBoost(double boost){
	return (boost + 6) / 6 - 1;
}

std::vector<double> addNormStats(json boosts, std::vector<double> prev){
	std::vector<double> tmp, res;
	tmp.push_back(normBoost(boosts["atk"]));
	tmp.push_back(normBoost(boosts["def"]));
	tmp.push_back(normBoost(boosts["spa"]));
	tmp.push_back(normBoost(boosts["spd"]));
	tmp.push_back(normBoost(boosts["spe"]));
	tmp.push_back(normBoost(boosts["accuracy"]));
	tmp.push_back(normBoost(boosts["evasion"]));
	res.reserve(prev.size() + tmp.size() ); // preallocate memory
	res.insert(res.end(), prev.begin(), prev.end() );
	res.insert(res.end(), tmp.begin(), tmp.end() );
	return res;
}

std::vector<double> addNormStats(json stats, int hp, std::vector<double> prev){
	std::vector<double> tmp, res;
	tmp.push_back(2 * double(hp) / double(stats["hp"]) - 1);
	tmp.push_back(normStat(stats["hp"]));
	tmp.push_back(normStat(stats["atk"]));
	tmp.push_back(normStat(stats["def"]));
	tmp.push_back(normStat(stats["spa"]));
	tmp.push_back(normStat(stats["spd"]));
	tmp.push_back(normStat(stats["spe"]));
	res.reserve(prev.size() + tmp.size() ); // preallocate memory
	res.insert(res.end(), prev.begin(), prev.end() );
	res.insert(res.end(), tmp.begin(), tmp.end() );
	return res;
}

std::vector<double> addStatusVector(json a, std::string query, std::vector<double> prev){
	std::vector<double> res;
	std::vector<double> a_vec(a.size(), -1);
	a_vec.at(a[query]) = 1;
	// Concatenate status vector with results vector
	res.reserve(prev.size() + a_vec.size() ); // preallocate memory
	res.insert(res.end(), prev.begin(), prev.end() );
	res.insert(res.end(), a_vec.begin(), a_vec.end() );
	return res;
}

std::vector<double> addTypeVector(json a, json types, std::vector<double> prev){
	std::vector<double> type_vector(18, -1);
	std::vector<double> res;
	for(std::string mon_type : a){
		type_vector.at(types[mon_type]) = 1;
	}
	// Concatenate types vector with results vector
	res.reserve(prev.size() + type_vector.size() ); // preallocate memory
	res.insert(res.end(), prev.begin(), prev.end() );
	res.insert(res.end(), type_vector.begin(), type_vector.end() );
	return res;
}
int main()
{
	//         return torch.Tensor(ability+item+types+moves+move_pp + move_disabled +status+stats
	//         +stat_boosts+sub_hp+trapped_counter+volatile_status)
	std::ifstream ifstate("turn_002.json");
	std::ifstream ifmoves("move_id.json");
	std::ifstream ifpokedex("pokedex.json");
	std::ifstream ifabilities("ability_id.json");
	std::ifstream ifitems("item_id.json");
	std::ifstream iftypes("type_id.json");
	std::ifstream ifstatus("status.json");
	json state, moves, pokedex, abilities, items, types, status;
	ifstate >> state;
	ifmoves >> moves;
	ifpokedex >> pokedex;
	ifabilities >> abilities;
	ifitems >> items;
	iftypes >> types;
	ifstatus >> status;
	std::vector<double> result;
	for (int side_i = 0; side_i < 2; side_i ++){
		for (int mon_i = 0; mon_i < int(state["sides"][side_i]["pokemon"].size()); mon_i++){
			json pokemon = state["sides"][side_i]["pokemon"][mon_i];
			// Append Pokemon ability
			std::string ability = pokemon["ability"];
			result.push_back(abilities[ability]);
			// Append Pokemon item
			std::string item = pokemon["item"];
			result.push_back(items[item]);
			// Append Pokemon types
			result = addTypeVector(pokemon["types"], types, result);

			// Append move_id, move pp, move max pp, and move status
			std::map<int, json> move_map;
			for (json move: pokemon["moveSlots"]){
				std::string move_id = move["id"];
				move_map.insert(std::make_pair(moves[move_id], move));
			}
			std::map<int, json>::iterator it = move_map.begin();
			while(it != move_map.end()) {
				result.push_back(it->first);
				result.push_back(2 * double((it->second)["pp"]) / double((it->second)["maxpp"]) - 1);
				result.push_back(2 * int((it->second)["disabled"]) - 1);
				it++;
			}
			// Append status
			result = addStatusVector(status, pokemon["status"], result);
			// Append stats
			result = addNormStats(pokemon["baseStoredStats"], pokemon["hp"], result);
			// Append stat boosts
			result = addNormStats(pokemon["boosts"], result);
			// Append trapped
			result.push_back(2 * int(pokemon["trapped"]) - 1);
		}
	}
	// Print out state vector
	for (const double& i : result) {
    	std::cerr << i << "  " << std::endl;
  	}
  return 0;
}
