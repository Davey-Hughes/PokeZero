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

#include <main.hh>

int
main()
{
	auto p1 = showdown::RandomPlayer("p1");
	auto p2 = showdown::RandomPlayer("p2");
	p1.connect(showdown::RandomPlayer::force_create);
	p2.connect(showdown::RandomPlayer::force_create);

	auto sd = showdown::Showdown();
	sd.start("./pokemon-showdown/.sim-dist/examples/battle-profiling.js", "p1", "p2");

	p1.loop();
	p2.loop();
}
