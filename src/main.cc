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

#include <iostream>
#include <future>
#include <sstream>
#include <random>
#include <functional>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/prettywriter.h"

#include <main.hh>

namespace rj = rapidjson;

pid_t
create_child(char *const argv[], char *const envp[], int fds[])
{
	/*
	 * parent writes into writepipe[1] and reads from readpipe[0]
	 * child writes into readpipe[1] and reads from writepipe[0]
	 */
	int readpipe[2], writepipe[2];
	pid_t pid;

	if (pipe(readpipe) < 0) {
		goto error_readpipe;
	}

	if (pipe(writepipe) < 0) {
		goto error_writepipe;
	}

        pid = fork();
	switch(pid) {
	case -1: // failed to fork child
		goto error_fork;
	case 0:  // child
		dup2(writepipe[0], STDIN_FILENO);
		close(writepipe[0]);
		dup2(readpipe[1], STDOUT_FILENO);
		close(readpipe[1]);

		if (execve(argv[0], argv, envp) == -1) {
			std::perror("Could not execve");
		}

		exit(-1);
	default: // parent
		close(writepipe[0]);
		close(readpipe[1]);
		break;
	}

	fds[0] = readpipe[0];
	fds[1] = writepipe[1];

	return pid;

error_fork:
	close(readpipe[0]);
	close(readpipe[1]);
error_writepipe:
	close(writepipe[0]);
	close(writepipe[1]);
error_readpipe:
	return -1;
}

int
read_child(int fd, std::atomic<bool> &quit)
{
	static char buf[BUFSIZE];
	ssize_t bytes_read;
	while (!quit && (bytes_read = read(fd, buf, BUFSIZE)) > 0) {
		/* echo the stdout of the child */
		write(STDOUT_FILENO, buf, bytes_read);
	}

	return 0;
}

void
write_child(int fd, std::atomic<bool> &quit)
{
	std::string input;
	while (!quit && std::getline(std::cin, input)) {
		input.push_back('\n');
		write(fd, input.c_str(), input.length());
	}
}

void
trivial_socket_client()
{
	std::string input;

	ssize_t bytes_written;
	int ret = 0;
	struct sockaddr_un addr;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, "/tmp/p1", sizeof(addr.sun_path) - 1);

	int sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket error");
		ret = -1;
		goto error_socket;
	}

	if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("connect error");
		ret = -2;
		goto error_connect;
	}

	while (std::getline(std::cin, input)) {
		rj::StringBuffer buf;
		rj::PrettyWriter<rj::StringBuffer> writer(buf);

		writer.StartObject();
		writer.String("input");
		writer.String(input);
		writer.EndObject();

		if ((bytes_written = write(sockfd, buf.GetString(), buf.GetLength())) == -1) {
			perror("error write");
			ret = -3;
			goto error_write;
		}
	}


error_write:
error_connect:
	close(sockfd);
error_socket:
	exit(ret);
}

std::string
json_to_string(const rj::Value &json)
{
	rj::StringBuffer sb;
	rj::PrettyWriter<rj::StringBuffer> writer(sb);
        json.Accept(writer);
        return sb.GetString();
}

int
choose_random(std::mt19937 &rng, int size)
{
	std::uniform_int_distribution<int> roll(0, size - 1);
	auto random_roll = std::bind(roll, std::ref(rng));
	return random_roll();
}

void
trivial_socket_server(std::string sock_path)
{
	char buf[BUFSIZE];
	int ret = 0;
	int rc, server_sock, client_sock;
	socklen_t len;
	ssize_t bytes_read;
	std::mt19937 rng(std::time(nullptr));

	struct sockaddr_un server_addr, client_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_LOCAL;
	strncpy(server_addr.sun_path, sock_path.c_str(), sizeof(server_addr.sun_path) - 1);

	server_sock = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (server_sock < 0) {
		perror("socket error");
		ret = -1;
		goto error_socket;
	}

	unlink(sock_path.c_str());
	rc = bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if (rc == -1) {
		perror("bind error");
		ret = -2;
		goto error_bind;
	}

	std::cout << "Listening..." << std::endl;
	rc = listen(server_sock, 1);
	if (rc == -1) {
		perror("listen error");
		ret = -3;
		goto error_listen;
	}

	client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &len);
	if (client_sock == -1) {
		perror("accept error");
		ret = -4;
		goto error_accept;
	}

	rc = getsockname(client_sock, (struct sockaddr *) &client_addr, &len);
	if (rc == -1) {
		perror("getsockname error");
		ret = -5;
		goto error_getsockname;
	}

	std::cout << "Client socket filepath: " << client_addr.sun_path << std::endl;

	while (1) {
		memset(buf, 0, BUFSIZE);
		std::stringstream ss;
		while ((bytes_read = recv(client_sock, buf, BUFSIZE, 0)) > 0) {
			ss << std::string_view(buf, bytes_read);

			if (bytes_read < BUFSIZE) {
				break;
			}
		}

		if (bytes_read == 0) {
			break;
		} else if (bytes_read == -1) {
			perror("recv error");
			ret = -6;
			goto error_recv;
		}

		/* TODO: check speed of this */
		rj::Document document;
		rj::IStreamWrapper rjss(ss);
		document.ParseStream(rjss);

		assert(document.HasMember("command"));
		assert(document["command"].IsString());
		std::string command(document["command"].GetString());

		rj::StringBuffer reply_buf;
		rj::PrettyWriter<rj::StringBuffer> writer(reply_buf);

		writer.StartObject();
		/* TODO: check speed of this */
		if (command.compare("active") == 0) {
			assert(document.HasMember("choices"));
			writer.String("active");
			writer.Int(choose_random(rng, document["choices"].GetArray().Size()));
		} else if (command.compare("forceSwitch") == 0) {
			assert(document.HasMember("choices"));
			writer.String("switch");
			writer.Int(choose_random(rng, document["choices"].GetArray().Size()));
		} else if (command.compare("teamPreview") == 0) {
			writer.String("preview");
			writer.String("default");
		} else {
			std::cerr << "Unknown command: " << command << std::endl;
		}
		writer.EndObject();

		rc = send(client_sock, reply_buf.GetString(), reply_buf.GetSize() + 1, 0);
		if (rc == -1) {
			perror("error send");
			ret = -7;
			goto error_send;
		}
	}

error_send:
error_recv:
error_getsockname:
error_accept:
	close(client_sock);
error_listen:
	unlink(sock_path.c_str());
error_bind:
	close(server_sock);
error_socket:
	exit(ret);
}

int
main(int argc, char **argv, char **envp)
{

	(void) argc;
	(void) argv;
	(void) envp;

	/* trivial_socket_client(); */
	auto p1_ret = std::async(trivial_socket_server, "/tmp/p1");
	auto p2_ret = std::async(trivial_socket_server, "/tmp/p2");

	int ret = 0;

	char *child_argv[] = {
		(char *) "/usr/local/bin/node",
		(char *) "./pokemon-showdown/.sim-dist/examples/battle-profiling.js",
		nullptr
	};
	int fds[2];

	pid_t childpid = create_child(child_argv, envp, fds);
	if (childpid < 0) {
		std::perror("Error creating child process");
		exit(-1);
	}

	std::atomic<bool> quit = false;

	auto read_ret = std::async(read_child, fds[0], std::ref(quit));
	auto write_ret = std::async(write_child, fds[1], std::ref(quit));

	int stat_loc;
	if (wait(&stat_loc) != childpid) {
		std::perror("Wait failed for child process");
		ret = -1;
		goto cleanup;
	}

	quit = true;

cleanup:
	close(fds[0]);
	close(fds[1]);

	return ret;
}
