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
	return (stat - 200.0) / 200.0;
}

std::vector<double> addNormStats(json stats, int hp, std::vector<double> prev){
	std::vector<double> tmp, res;
	tmp.push_back(double(hp) / double(stats["hp"]));
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
	std::vector<double> a_vec(a.size(), 0);
	a_vec.at(a[query]) = 1;
	// Concatenate status vector with results vector
	res.reserve(prev.size() + a_vec.size() ); // preallocate memory
	res.insert(res.end(), prev.begin(), prev.end() );
	res.insert(res.end(), a_vec.begin(), a_vec.end() );
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
	std::vector<double> tmp, tmp2;
	// Append Pokemon ability
	std::string ability = state["sides"][0]["pokemon"][0]["ability"];
	tmp.push_back(abilities[ability]);
	// Append Pokemon item
	std::string item = state["sides"][0]["pokemon"][0]["item"];
	tmp.push_back(items[item]);
	// Append Pokemon types
	std::vector<double> type_vector(18, 0);
	for(std::string mon_type : state["sides"][0]["pokemon"][0]["types"]){
		type_vector.at(types[mon_type]) = 1;
	}
	// Concatenate types vector with results vector
	tmp2.reserve(tmp.size() + type_vector.size() ); // preallocate memory
	tmp2.insert(tmp2.end(), tmp.begin(), tmp.end() );
	tmp2.insert(tmp2.end(), type_vector.begin(), type_vector.end() );

	// Append move_id, move pp, move max pp, and move status
	std::map<int, json> move_map;
	for (json move: state["sides"][0]["pokemon"][0]["moveSlots"]){
		std::string move_id = move["id"];
		move_map.insert(std::make_pair(moves[move_id], move));
	}
	std::map<int, json>::iterator it = move_map.begin();
    while(it != move_map.end()) {
		tmp2.push_back(it->first);
		tmp2.push_back(double((it->second)["pp"]) / double((it->second)["maxpp"]));
		tmp2.push_back(int((it->second)["disabled"]));
        it++;
    }
	// Append status
	tmp2 = addStatusVector(status, state["sides"][0]["pokemon"][0]["status"], tmp2);
	tmp2 = addNormStats(state["sides"][0]["pokemon"][0]["baseStoredStats"], state["sides"][0]["pokemon"][0]["hp"], tmp2);

	// Print out state vector
	for (const double& i : tmp2) {
    	std::cerr << i << "  " << std::endl;
  	}
  return 0;
}
