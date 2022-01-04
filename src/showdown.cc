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

#include "showdown.hh"

#include <sys/wait.h>
#include <unistd.h>

#include <string>
#include <vector>

namespace showdown {

/*
 * default constructor
 */
Showdown::Showdown() {}

/*
 * destructor
 */
Showdown::~Showdown()
{
	/* TODO: more error handling? */
	int stat_loc;
	if (waitpid(this->child.pid, &stat_loc, 0) != this->child.pid) {
		std::perror("Wait failed for child process");
	}
}

/*
 * start the node process
 */
bool
Showdown::start(const std::string &showdown_script, const std::vector<std::string> &extra_args)
{
	this->showdown_script = showdown_script;
	bool success = true;

	/*
	 * create vector of arguments with "node", the node script to run, and
	 * any extra arguments to pass to node
	 */
	std::vector<std::string> argv_builder{"node", this->showdown_script};
	argv_builder.insert(argv_builder.end(), extra_args.begin(), extra_args.end());

	/* build a char ** that execvp() accepts from the vector of arguments */
	char const **argv = new char const *[argv_builder.size() + 1];
	for (size_t i = 0; i < argv_builder.size(); ++i) {
		argv[i] = argv_builder[i].c_str();
	}

	/* make last element nullptr */
	argv[argv_builder.size()] = nullptr;

	/*
	 * parent writes into writepipe[1] and reads from readpipe[0]
	 * child writes into readpipe[1] and reads from writepipe[0]
	 */
	int readpipe[2], writepipe[2];
	pid_t pid;

	if (pipe(readpipe) < 0) {
		success = false;
		goto cleanup_readpipe;
	}

	if (pipe(writepipe) < 0) {
		success = false;
		goto cleanup_writepipe;
	}

	pid = fork();
	switch (pid) {
	case -1: // failed to fork child
		success = false;
		goto cleanup_fork;
	case 0: // child
		/* dup2(writepipe[0], STDIN_FILENO); */
		close(writepipe[0]);
		/* dup2(readpipe[1], STDOUT_FILENO); */
		close(readpipe[1]);

		if (execvp(argv[0], (char *const *) argv) == -1) {
			std::perror("Could not execvp");
		}

		exit(-1);
	default: // parent
		break;
	}

	this->child.pid = pid;
	this->child.readfd = readpipe[0];
	this->child.writefd = writepipe[1];

cleanup_fork:
	close(readpipe[0]);
	close(readpipe[1]);
cleanup_writepipe:
	close(writepipe[0]);
	close(writepipe[1]);
cleanup_readpipe:
	delete[] argv;
	return success;
}
} // namespace showdown
