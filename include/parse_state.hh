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
#include <json.hpp>
#include <map>
#include <iterator>

namespace parse_state {
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

std::vector<double> concatVectors(std::vector<double> prev, std::vector<double> tmp){
	std::vector<double> res;
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
	res = concatVectors(prev, tmp);
	return res;
}

std::vector<double> addStatusVector(json a, std::string query, std::vector<double> prev){
	std::vector<double> res;
	std::vector<double> a_vec(a.size(), -1);
	a_vec.at(a[query]) = 1;
	// Concatenate status vector with results vector
	res = concatVectors(prev, a_vec);
	return res;
}

std::vector<double> addTypeVector(json a, json types, std::vector<double> prev){
	std::vector<double> type_vector(types.size(), -1);
	std::vector<double> res;
	for(std::string mon_type : a){
		if (types.contains(mon_type)){
			type_vector.at(types[mon_type]) = 1;
		}
	}
	// Concatenate types vector with results vector
	res = concatVectors(prev, type_vector);
	return res;
}

std::vector<double> addVolVector(json a, json volatiles, std::vector<double> prev){
	std::vector<double> vol_vector(volatiles.size(), -1);
	std::vector<double> res;
	if(a.size() > 0){
		for(json vol : a){
			if (volatiles.contains(vol["id"])){
				std::string vol_id = vol["id"];
				vol_vector.at(volatiles[vol_id]) = 1;
			}
		}
	}
	// Concatenate types vector with results vector
	res = concatVectors(prev, vol_vector);
	return res;
}

std::vector<double> addGameVector(json a, json weather, std::vector<double> prev){
	std::vector<double> res;
	std::vector<double> weather_vector(weather.size(), -1);
	double weather_d = -1;
	if(a.size() > 1){
		std::string w_id = a["id"];
		weather_vector.at(weather[w_id]) = 1;
		weather_d = double(a["duration"]) / 7 * 2 - 1;
	}
	res = concatVectors(prev, weather_vector);
	res.push_back(weather_d);
	return res;
}
std::vector<double> parseState() {
	//         return torch.Tensor(ability+item+types+moves+move_pp + move_disabled +status+stats
	//         +stat_boosts+sub_hp+trapped_counter+volatile_status)
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
	json state, moves, pokedex, abilities, items, types, status, volatiles, sideconds, slotconds, terrain, weather;
	ifstate >> state;
	std::cerr << "MADE IT HERE" << std::endl;	
	ifmoves >> moves;
	ifpokedex >> pokedex;
	ifabilities >> abilities;
	ifitems >> items;
	iftypes >> types;
	ifstatus >> status;
	ifvolatiles >> volatiles;
	ifsideconds >> sideconds;
	ifslotconds >> slotconds;
	ifterrain >> terrain;
	ifweather >> weather;
	std::vector<double> result, tmp;

	for (int side_i = 0; side_i < 2; side_i ++){
		// Iterate through map of Pokemon
		std::map<std::string, json> poke_map;
		for (json mon: state["sides"][side_i]["pokemon"]){
			std::string poke_id = mon["speciesState"]["id"];
			poke_map.insert(std::make_pair(poke_id, mon));
		}
		std::map<std::string, json>::iterator poke_it = poke_map.begin();
		while(poke_it != poke_map.end()) {
			json pokemon = poke_it -> second;
			
			// Append their active status
			result.push_back(int(pokemon["isActive"]));
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
			std::map<int, json>::iterator move_it = move_map.begin();
			while(move_it != move_map.end()) {
				result.push_back(move_it->first);
				result.push_back(2 * double((move_it->second)["pp"]) / double((move_it->second)["maxpp"]) - 1);
				result.push_back(2 * int((move_it->second)["disabled"]) - 1);
				move_it++;
			}
			// Append status
			result = addStatusVector(status, pokemon["status"], result);
			// Append stats
			result = addNormStats(pokemon["baseStoredStats"], pokemon["hp"], result);
			// Append stat boosts
			result = addNormStats(pokemon["boosts"], result);
			// Append trapped
			result.push_back(2 * int(pokemon["trapped"]) - 1);
			// Append Pokemon volatiles if active
			if (pokemon["isActive"]){
				tmp = addVolVector(pokemon["volatiles"], volatiles, tmp);
			}
			poke_it++;
		}
		result = concatVectors(result, tmp);
		tmp.clear();

		// Append side conditions
		std::vector<double> sideconds_vector(sideconds.size(), -1);
		double spikes_ctr = -1, tspikes_ctr = spikes_ctr, ref_ctr = spikes_ctr, ls_ctr = spikes_ctr, av_ctr = spikes_ctr, tw_ctr = spikes_ctr;
		if(state["sides"][side_i]["sideConditions"].size() > 0){
			for(json cond : state["sides"][side_i]["sideConditions"]){
				if (sideconds.contains(cond["id"])){
					std::string cond_id = cond["id"];
					sideconds_vector.at(sideconds[cond_id]) = 1;
					if (cond_id == "spikes"){
						spikes_ctr = double(sideconds["spikes"]["layers"]) / 3 * 2 - 1;
					}
					else if (cond_id == "toxicspikes"){
						tspikes_ctr = double(sideconds["toxicspikes"]["layers"]) - 1;
					}
					else if (cond_id == "reflect"){
						ref_ctr = double(sideconds["reflect"]["duration"]) / 7 * 2 - 1;
					}
					else if (cond_id == "lightscreen"){
						ls_ctr = double(sideconds["lightscreen"]["duration"]) / 7 * 2 - 1;
					}
					else if (cond_id == "auroraveil"){
						av_ctr = double(sideconds["auroraveil"]["duration"]) / 7 * 2 - 1;
					}
					else if (cond_id == "tailwind"){
						tw_ctr = double(sideconds["tailwind"]["duration"]) / 7 * 2 - 1;
					}
				}
			}
		}
		result = concatVectors(result, sideconds_vector);
		result.push_back(spikes_ctr);
		result.push_back(tspikes_ctr);
		result.push_back(ref_ctr);
		result.push_back(ls_ctr);
		result.push_back(av_ctr);
		result.push_back(tw_ctr);

		// Append slot conditions (future sight, wish)
		std::vector<double> slotconds_vector(slotconds.size(), -1);
		double wish_d = -1, wish_hp = wish_d, future_move = wish_d, future_d = wish_d; 
		if(state["sides"][side_i]["slotConditions"].size() > 0){
			for(json cond : state["sides"][side_i]["slotConditions"]){
				if (slotconds.contains(cond["id"])){
					std::string cond_id = cond["id"];
					slotconds_vector.at(slotconds[cond_id]) = 1;
					if (cond_id == "wish"){
						wish_d = double(slotconds["wish"]["duration"]) * 2 - 1 ;
						wish_hp = normStat(double(slotconds["wish"]["hp"]));
					}
					else if (cond_id == "futuremove"){
						future_move = 1;
						future_d = double(slotconds["futuremove"]["duration"]) - 1;
					}
					
				}
			}
		}
		result = concatVectors(result, slotconds_vector);
		result.push_back(wish_d);
		result.push_back(wish_hp);
		result.push_back(future_move);
		result.push_back(future_d);
	}

	// Append weather
	result = addGameVector(state["field"]["weatherState"], weather, result);
	// Append terrain
	result = addGameVector(state["field"]["terrainState"], terrain, result);

	// Append trick room
	double tr = -1, tr_ctr = tr;
	if (state["field"]["pseudoWeather"].size() > 0){
		for(json pweather : state["field"]["pseudoWeather"]){
			if (pweather["id"] == "trickroom"){
				tr = 1;
				tr_ctr = pweather["duration"];
			}	
		}
	}
	result.push_back(tr);
	result.push_back(tr_ctr);

	// Print out state vector
	for (const double& i : result) {
    	std::cerr << i << "  " << std::endl;
  	}
	std::cerr << result.size() << std::endl;
  return result;
}
}

#endif /* PARSE_STATE_HH */
