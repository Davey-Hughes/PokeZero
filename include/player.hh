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

#include <sys/socket.h>
#include <unistd.h>

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>

#define RECV_BUFSIZE 4096

namespace showdown {
struct Client {
public:
	std::string name;
	std::atomic_int sockfd = -1;
	std::mutex socket_lock;
	std::condition_variable socket_ready;

	void Close()
	{
		std::unique_lock<std::mutex> lk(socket_lock);
		if (this->sockfd > -1)
			close(this->sockfd);
		this->sockfd = -1;
		lk.unlock();
	}

	/*
	 * read from socket into a stringstream until a null byte is found,
	 * then return a string up until the message
	 *
	 * the leftover bytes read (if any) are left in the recv_stream for the
	 * next call to RecvMessage()
	 */
	std::string RecvMessage()
	{
		if (this->sockfd < 0) {
			std::unique_lock<std::mutex> lk(this->socket_lock);
			while (this->sockfd < 0) {
				this->socket_ready.wait(lk);
			}
			lk.unlock();
		}

		char buf[RECV_BUFSIZE];
		ssize_t bytes_read;
		size_t message_len = 0;

		while ((bytes_read = recv(this->sockfd, buf, RECV_BUFSIZE, 0)) > 0) {
			recv_stream << std::string_view(buf, bytes_read);
			size_t msg_end = std::string_view(buf, bytes_read).find('\0');
			if (msg_end != std::string::npos) {
				message_len += msg_end;
				std::string message(message_len, '\0');
				recv_stream.read(&message[0], message_len);
				recv_stream.seekg(1, std::ios_base::cur);

				return message;
			}
			message_len += bytes_read;
		}

		if (bytes_read < 0) {
			/* TODO: handle error better? */
			perror("client socket recv error");
		}

		/* only get here on recv error or close */
		Close();
		return "";
	}

	~Client() { Close(); }

private:
	std::stringstream recv_stream;
};

class Player {
public:
	virtual void loop() = 0;

	[[maybe_unused]] static constexpr bool force_create = true;

	/* constructor */
	Player(const std::string &);

	/* destructor */
	~Player();

	void connect(bool force = false);

protected:
	std::string name = "";
	std::string socket_name = "";
	int server_sockfd = -1;
	std::mt19937 rng;
	Client client;
	std::vector<std::thread> threads;

	int createSocket(bool force = false);
	bool connectHelper();
	void closeSocketServer();
};
} // namespace showdown

#endif /* PLAYER_HH */
