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

#include "socket_helper.hh"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace showdown {
/*
 * create a socket to listen for connections on
 */
int
Socket::createSocket(const std::string &socket_name, bool force)
{
	int err = 0;
	int server_sockfd;

	struct sockaddr_un server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_LOCAL;
	strncpy(server_addr.sun_path, socket_name.c_str(), sizeof(server_addr.sun_path) - 1);

	if (force) {
		unlink(this->socket_name.c_str());
	}

	server_sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (server_sockfd < 0) {
		perror((socket_name + ": socket error").c_str());
		err = server_sockfd;
		goto cleanup_socket;
	}

	err = bind(server_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if (err) {
		perror((socket_name + ": bind error").c_str());
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
Socket::closeServer()
{
	if (this->server_sockfd > -1) {
		close(this->server_sockfd);
	}
	this->server_sockfd = -1;

	unlink(this->socket_name.c_str());
}

void
Socket::connect(bool force)
{
	if (this->server_sockfd == -1) {
		this->createSocket(this->socket_name, force);
	}

	this->threads.push_back(std::thread([this]() { this->connectHelper(); }));
}

bool
Socket::connectHelper()
{
	int err, client_sockfd;
	struct sockaddr_un client_addr;
	socklen_t len;
	std::unique_lock<std::mutex> lk(this->socket_lock, std::defer_lock);

	err = listen(this->server_sockfd, 1);
	if (err == -1) {
		perror((this->socket_name + ": listen error").c_str());
		goto cleanup_listen;
	}

	client_sockfd = accept(this->server_sockfd, (struct sockaddr *) &client_addr, &len);
	if (client_sockfd == -2) {
		perror((this->socket_name + ": accept error").c_str());
		goto cleanup_accept;
	}

	err = getsockname(client_sockfd, (struct sockaddr *) &client_addr, &len);
	if (err == -1) {
		perror("getsockname error");
		goto cleanup_getsockname;
	}

	lk.lock();
	this->sockfd = client_sockfd;
	lk.unlock();
	this->socket_ready.notify_one();

	return true;

cleanup_getsockname:
	close(client_sockfd);
cleanup_accept:
cleanup_listen:
	return false;
}

/*
 * read from socket into a stringstream until a null byte is found,
 * then return a string up until the message
 *
 * the leftover bytes read (if any) are left in the recv_stream for the
 * next call to recvMessage()
 */
std::string
Socket::recvMessage()
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
		// TODO: handle error better?
		perror("client socket recv error");
	}

	// only get here on recv error or close
	closeClient();
	return "";
}

// TODO: handle error better
void
Socket::sendMessage(const std::string &msg)
{
	if (this->sockfd < 0) {
		std::unique_lock<std::mutex> lk(this->socket_lock);
		while (this->sockfd < 0) {
			this->socket_ready.wait(lk);
		}
		lk.unlock();
	}

	ssize_t bytes_sent;
	bytes_sent = send(this->sockfd, msg.c_str(), msg.length() + 1, 0);
	if (bytes_sent == -1) {
		// TODO: handle better
		perror("error send");
		return;
	}
}

void
Socket::closeClient()
{
	std::unique_lock<std::mutex> lk(socket_lock);
	if (this->sockfd > -1)
		close(this->sockfd);
	this->sockfd = -1;
	lk.unlock();
}

/*
 * destructor
 */
Socket::~Socket()
{
	for (auto &t: this->threads) {
		t.join();
	}
	this->threads.clear();

	closeClient();

	this->closeServer();
}

} // namespace showdown
