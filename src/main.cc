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
#include <unistd.h>
#include <fcntl.h>

#include <main.hh>

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

int
main(int argc, char **argv, char *envp[])
{
	(void) argc;
	(void) argv;

	int ret = 0;

	char node[] = "/usr/local/bin/node";
	char js_file[] = "./pokemon-showdown/.sim-dist/examples/battle-profiling.js";
	char *child_argv[] = {node, js_file, nullptr};
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
	read_ret.wait();
	write_ret.wait();

cleanup:
	close(fds[0]);
	close(fds[1]);

	return ret;
}
