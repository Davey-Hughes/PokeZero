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

#include <rapidjson/document.h>
#include "rapidjson/prettywriter.h"

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

			/* TODO: check speed of this */
			rapidjson::Document document;
			document.Parse(message.c_str());

			assert(document.HasMember("command"));
			assert(document["command"].IsString());
			std::string command(document["command"].GetString());

			rapidjson::StringBuffer reply_buf;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(reply_buf);

			writer.StartObject();
			/* TODO: check speed of this */
			if (command.compare("active") == 0) {
				assert(document.HasMember("choices"));
				writer.String("active");
				writer.Int(this->randomInt(0, document["choices"].GetArray().Size() - 1));
			} else if (command.compare("forceSwitch") == 0) {
				assert(document.HasMember("choices"));
				writer.String("switch");
				writer.Int(this->randomInt(0, document["choices"].GetArray().Size() - 1));
			} else if (command.compare("teamPreview") == 0) {
				writer.String("preview");
				writer.String("default");
			} else {
				std::cerr << "Unknown command: " << command << std::endl;
			}
			writer.EndObject();

			bytes_sent = send(this->client.sockfd, reply_buf.GetString(), reply_buf.GetSize() + 1, 0);
			if (bytes_sent == -1) {
				perror("error send");
				goto error_send;
			}
		}

	error_send:
		return;
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
