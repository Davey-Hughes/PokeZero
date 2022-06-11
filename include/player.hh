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

#ifndef PLAYER_HH
#define PLAYER_HH

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <random>
#include <string>

#include "socket_helper.hh"

namespace showdown {
enum MoveType {
	WAIT,    // tell the player class to wait for directive
	OWN,     // let player class make move
	DIRECTED // use move from caller
};

class Player {
public:
	[[maybe_unused]] static constexpr bool force_create = true;
	std::string name = "";
	std::atomic<MoveType> move_type{WAIT};

	// constructor
	Player(const std::string &, const std::string &);

	// destructor
	virtual ~Player(){};

	virtual void loop() = 0;
	void notifyMove(MoveType, const std::string &);
	void notifyOwnMove();
	void requestSetExit();

protected:
	std::string className;

	Socket socket;

	std::mt19937 rng;

	std::string move;
	std::mutex move_lock;
	std::condition_variable move_cv;

	std::string waitDirectedMove();
	virtual std::string decideOwnMove() { return ""; };
};
} // namespace showdown

#endif /* PLAYER_HH */
