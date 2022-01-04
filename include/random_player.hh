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

#ifndef RANDOM_PLAYER_HH
#define RANDOM_PLAYER_HH

#include <thread>
#include <vector>

#include "player.hh"

namespace showdown {
class RandomPlayer : public Player {
public:
	/* inherit constructors */
	using Player::Player;

	void loop();

	~RandomPlayer();

private:
	size_t randomInt(size_t, size_t);
	std::vector<std::thread> threads;
};
} // namespace showdown

#endif /* RANDOM_PLAYER_HH */
