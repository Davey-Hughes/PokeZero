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

#include "player.hh"

#include <string.h>
#include <unistd.h>

#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <random>
#include <stdexcept>

#include "common.hh"

namespace showdown {
/*
 * constructor
 */
Player::Player(const std::string &name, const std::string &className)
{
	this->className = className;

	validateName(name);
	this->name = name;
	this->socket.socket_name = "/tmp/" + name;

	uint32_t seed = 0;
	char *buffer = reinterpret_cast<char *>(&seed);
	std::ifstream urandom_stream("/dev/urandom", std::ios_base::binary | std::ios_base::in);
	urandom_stream.read(buffer, sizeof(seed));

	std::mt19937 rand(seed);

	this->socket.connect(true);
}

Player::~Player()
{
	for (auto &t: this->threads) {
		t.join();
	}

	this->threads.clear();
}

void
Player::loop()
{
	this->threads.push_back(std::thread([this]() {
		while (1) {
			std::string message = this->socket.recvMessage();

			if (message.empty()) {
				return;
			}

			this->last_request = nlohmann::json::parse(message);

			std::string reply_str = this->waitDirectedMove();
			this->socket.sendMessage(reply_str);
		}
	}));
}

/*
 * let the controlling thread tell this class to decide its own move or use the given move
 */
void
Player::notifyMove(MoveType move_type, const std::string &move)
{
	// TODO ensure move is valid if move_type is DIRECTED?
	std::unique_lock<std::mutex> lk(this->move_lock);
	this->move = move;
	this->move_type = move_type;
	lk.unlock();

	this->move_cv.notify_one();
}

/*
 * alias for letting this class make its own move
 */
void
Player::notifyOwnMove()
{
	this->notifyMove(MoveType::OWN, "");
}

std::string
Player::waitDirectedMove()
{
	if (this->move_type == WAIT) {
		std::unique_lock<std::mutex> lk(this->move_lock);
		while (this->move_type == WAIT) {
			// std::cout << this->name << " waiting" << std::endl;
			this->move_cv.wait(lk);
		}
		lk.unlock();
	}

	switch (this->move_type) {
	case OWN:
		this->move_type = WAIT;
		return decideOwnMove();
	case DIRECTED:
		this->move_type = WAIT;
		return this->move;
	case WAIT: // shouldn't get here
		throw std::runtime_error("Shouldn't reach case WAIT in waitDirectedMove");
	}
}
} // namespace showdown
