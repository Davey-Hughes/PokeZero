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

#ifndef MANAGER_HH
#define MANAGER_HH

#include <nlohmann/json.hpp>
#include <thread>

#include "battle_parser.hh"
#include "common.hh"
#include "player.hh"
#include "random_player.hh"
#include "showdown.hh"
#include "socket_helper.hh"

namespace pokezero {
template <class P1 = showdown::RandomPlayer, class P2 = showdown::RandomPlayer>
class Manager {
public:
	// constructors
	Manager(const std::string &);

	// destructor
	~Manager();

	void start();
	virtual void loop();

protected:
	std::string name;

	showdown::Showdown sd;
	BattleParser parser = BattleParser();
	std::array<showdown::Player *, 2> players;

	std::vector<std::thread> threads;

	showdown::Socket socket;

	void requestGetBattleState(int);
	void requestSetBattleState(int);
	void requestSetExit();
};

/*
 * constructor
 */
template <class P1, class P2>
Manager<P1, P2>::Manager(const std::string &name)
{
	validateName(name);
	this->name = name;
	this->socket.socket_name = "/tmp/" + name;

	this->players[0] = new P1("p1");
	this->players[1] = new P2("p2");
}

/*
 * destructor
 */
template <class P1, class P2>
Manager<P1, P2>::~Manager()
{
	for (auto &t: this->threads) {
		t.join();
	}

	this->threads.clear();

	for (auto &p: this->players) {
		delete p;
	}
}

/*
 * start the node instance and players and run the game
 */
template <class P1, class P2>
void
Manager<P1, P2>::start()
{
	// TODO: don't force unlink of socket file?
	this->socket.connect(true);

	this->sd = showdown::Showdown();

	for (auto &p: this->players) {
		p->loop();
	}

	// TODO: encode path better
	// this->sd.start("./pokemon-showdown/.sim-dist/examples/battle-managing.js", this->name);
	this->sd.start("./pokemon-showdown/.sim-dist/examples/battle-managing.js", this->name, this->players[0]->name,
	               this->players[1]->name);

	this->loop();
}

/*
 * send a request to the node process to get the battle state corresponding to
 * the specified turn
 */
template <class P1, class P2>
void
Manager<P1, P2>::requestGetBattleState(int turn)
{
	nlohmann::json send_msg;

	send_msg["method"] = "get";
	send_msg["item"] = "battleState";
	send_msg["turn"] = turn;

	this->socket.sendMessage(send_msg.dump());
}

/*
 * send a request to the node process to set the battle state corresponding to
 * the specified turn
 *
 * this can be used to reset the game by setting the turn to 0
 */
template <class P1, class P2>
void
Manager<P1, P2>::requestSetBattleState(int turn)
{
	nlohmann::json send_msg;

	send_msg["method"] = "set";
	send_msg["item"] = "battleState";
	send_msg["turn"] = turn;

	this->socket.sendMessage(send_msg.dump());
}

template <class P1, class P2>
void
Manager<P1, P2>::requestSetExit()
{
	nlohmann::json send_msg;

	send_msg["method"] = "set";
	send_msg["item"] = "exit";

	this->socket.sendMessage(send_msg.dump());
	this->socket.closeClient();
}

template <class P1, class P2>
void
Manager<P1, P2>::loop()
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

			for (auto &p: this->players) {
				p->notifyOwnMove();
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

#endif /* MANAGER_HH */
