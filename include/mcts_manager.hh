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

#ifndef MCTS_MANAGER_HH
#define MCTS_MANAGER_HH

#include <string>

#include "manager.hh"
#include "player.hh"
#include "random_player.hh"

namespace pokezero {
template <class P1, class P2>
class MCTSManager : public Manager<P1, P2> {
public:
	// constructor
	using Manager<P1, P2>::Manager;

private:
	void loop() override;
	void renameMe();
};

template <class P1, class P2>
void
MCTSManager<P1, P2>::renameMe()
{
	return;
}

template <class P1, class P2>
void
MCTSManager<P1, P2>::loop()
{
	this->threads.push_back(std::thread([this]() {
		int turn = 0;
		while (true) {
			this->requestGetBattleState(turn);
			std::string res = this->socket.recvMessage();

			if (res.empty()) {
				// shouldn't get here in proper usage
				throw std::runtime_error("Socket closed before receiving Battle END");
			}

			this->renameMe();

			for (auto &p: this->players) {
				p->notifyMove(showdown::MoveType::DIRECTED, "replace_me");
			}

			switch (this->parser.handleResponse(res)) {
			case BattleParser::END:
				this->requestSetExit();
				return;
			case BattleParser::EMPTY:
				// do nothing
				break;
			case BattleParser::BATTLESTATE:
				this->parser.getMLVec(turn);
				turn++;
				break;
			}
		};
	}));
}
} // namespace pokezero

#endif /* MCTS_MANAGER_HH */
