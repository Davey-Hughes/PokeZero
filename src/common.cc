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

#include "common.hh"

#include <unistd.h>

#include <algorithm>
#include <functional>
#include <random>
#include <stdexcept>
#include <string>

/*
 * player name must be alphanumeric and up to MAX_NAME_LENGTH characters
 */
void
validateName(const std::string &name)
{
	if (name.length() > MAX_NAME_LENGTH) {
		throw std::invalid_argument(name + " name has max length: " + std::to_string(MAX_NAME_LENGTH));
	}

	for (auto &c: name) {
		if (!std::isalnum(c)) {
			throw std::invalid_argument(name + " name must be alphanumeric");
		}
	}
}

/*
 * from https://stackoverflow.com/a/12468109
 */
std::string
randomString(std::mt19937 &rng, socklen_t length)
{
	static std::string charset =
		"0123456789"
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	thread_local std::uniform_int_distribution<std::string::size_type> pick(0, charset.size() - 2);

	std::string s;
	s.reserve(length);

	while (length--) {
		s += charset[pick(rng)];
	}

	return s;
}
