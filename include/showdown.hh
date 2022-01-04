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

#ifndef SHOWDOWN_HH
#define SHOWDOWN_HH

#include <string>
#include <vector>

namespace showdown {
class Showdown {
public:
	/* constructors */
	Showdown();
	~Showdown();

	bool start(const std::string &, const std::vector<std::string> &);
	bool start(const std::string &) { return this->start(showdown_script, {}); }

	template <typename... Ts>
	bool start(const std::string &showdown_script, Ts &...args)
	{
		return this->start(showdown_script, {args...});
	}

private:
	std::string showdown_script = "";

	struct {
		pid_t pid = 0;
		int readfd = -1;
		int writefd = -1;
	} child;
};
} // namespace showdown

#endif /* SHOWDOWN_HH */
