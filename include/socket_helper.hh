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

#ifndef SOCKET_HELPER_HH
#define SOCKET_HELPER_HH

#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#define RECV_BUFSIZE 4096

namespace showdown {
struct Socket {
public:
	std::atomic_int sockfd = -1;
	std::mutex socket_lock;
	std::condition_variable socket_ready;
	std::string socket_name = "";

	int server_sockfd = -1;

	int createSocket(const std::string &, bool force = false);
	void closeServer();

	void connect(bool force = false);
	bool connectHelper();
	std::string recvMessage();
	void sendMessage(const std::string &msg);
	void closeClient();

	~Socket();

private:
	std::stringstream recv_stream;
	std::vector<std::thread> threads;
};

struct SocketClientHandler {
public:

private:
};

} // namespace showdown

#endif /* SOCKET_HELPER_HH */
