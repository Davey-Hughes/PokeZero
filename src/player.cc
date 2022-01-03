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

#include <fstream>
#include <locale>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <unistd.h>
#include <string.h>
#include <random>
#include <sys/socket.h>
#include <sys/un.h>

#define MAX_NAME_LENGTH 0xff

/*
 * constructor
 *
 * player name must be alphanumeric and up to MAX_NAME_LENGTH characters
 */
Player::Player(const std::string &name)
{
	if (name.length() > MAX_NAME_LENGTH) {
		throw std::invalid_argument("Player name has max length: " + std::to_string(MAX_NAME_LENGTH));
	}

	for (auto &c: name) {
		if (!std::isalnum(c)) {
			throw std::invalid_argument("Player name must be alphanumeric");
		}
	}

	this->name = name;
	this->socket_name = "/tmp/" + name;

	uint32_t seed = 0;
	char *buffer = reinterpret_cast<char *>(&seed);
	std::ifstream urandom_stream("/dev/urandom", std::ios_base::binary | std::ios_base::in);
	urandom_stream.read(buffer, sizeof(seed));

	std::mt19937 rand(seed);
}

/*
 * destructor
 */
Player::~Player()
{
	this->closeSocketServer();
	unlink(this->socket_name.c_str());

	for (auto &t: this->threads) {
		t.join();
	}
}

void
Player::connect(bool force)
{
	if (this->server_sockfd == -1) {
		this->createSocket(force);
	}

	this->threads.push_back(std::thread([this]() { this->connectHelper(); }));
}

bool
Player::connectHelper()
{
	int err, client_sockfd;
	struct sockaddr_un client_addr;
	socklen_t len;
	std::unique_lock<std::mutex> lk(this->client.socket_lock, std::defer_lock);

	err = listen(this->server_sockfd, 1);
	if (err == -1) {
		perror("listen error");
		goto cleanup_listen;
	}

	client_sockfd = accept(this->server_sockfd, (struct sockaddr *) &client_addr, &len);
	if (client_sockfd == -2) {
		perror("accept error");
		goto cleanup_accept;
	}

	err = getsockname(client_sockfd, (struct sockaddr *) &client_addr, &len);
	if (err == -1) {
		perror("getsockname error");
		goto cleanup_getsockname;
	}

	lk.lock();
	this->client.sockfd = client_sockfd;
	lk.unlock();
	this->client.socket_ready.notify_one();

	this->client.name = client_addr.sun_path;

	return true;

cleanup_getsockname:
	close(client_sockfd);
cleanup_accept:
cleanup_listen:
	return false;
}

/*
 * create a socket to listen for connections on
 */
int
Player::createSocket(bool force)
{
	int err = 0;
	int server_sockfd;

	struct sockaddr_un server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_LOCAL;
	strncpy(server_addr.sun_path, this->socket_name.c_str(), sizeof(server_addr.sun_path) - 1);

	if (force) {
		unlink(this->socket_name.c_str());
	}

	server_sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (server_sockfd < 0) {
		perror("socket error");
		err = server_sockfd;
		goto cleanup_socket;
	}

	err = bind(server_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if (err) {
		perror("bind error");
		goto cleanup_bind;
	}

	this->server_sockfd = server_sockfd;
	return server_sockfd;

cleanup_bind:
	close(server_sockfd);
cleanup_socket:
	return err;
}

/*
 * close the server socket if initialized
 */
void
Player::closeSocketServer()
{
	if (this->server_sockfd > -1) {
		close(this->server_sockfd);
	}

	this->server_sockfd = -1;
}