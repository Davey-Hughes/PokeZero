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

#include "random_player.hh"

#include <vector>
#include <thread>

#include <nlohmann/json.hpp>

namespace showdown {

RandomPlayer::~RandomPlayer()
{
	for (auto &t: this->threads) {
		t.join();
	}
}

void
RandomPlayer::loop()
{
	this->threads.push_back(std::thread([this]() {
		while (1) {
			ssize_t bytes_sent;

			std::string message = client.RecvMessage();

			if (message.empty()) {
				return;
			}

			nlohmann::json msg_json = nlohmann::json::parse(message);
			std::string command = msg_json["command"];
			nlohmann::json reply;

			if (command == "active") {
				reply["active"] = this->randomInt(0, msg_json["choices"].size() - 1);
			} else if (command == "forceSwitch") {
				reply["switch"] = this->randomInt(0, msg_json["choices"].size() - 1);
			} else if (command == "teamPreview") {
				reply["teamPreview"] = "default";
			} else {
				std::cerr << "Unknown command: " << command << std::endl;
			}

			std::string reply_str = reply.dump();
			bytes_sent = send(this->client.sockfd, reply_str.c_str(), reply_str.length() + 1, 0);
			if (bytes_sent == -1) {
				/* TODO: handle better */
				perror("error send");
				return;
			}
		}
	}));
}

size_t
RandomPlayer::randomInt(size_t start, size_t end)
{
	std::uniform_int_distribution<size_t> roll(start, end);
	auto random_roll = std::bind(roll, std::ref(this->rng));
	return random_roll();
}
}
