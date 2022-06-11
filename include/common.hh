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

#ifndef COMMON_HH
#define COMMON_HH

#include <sys/socket.h>
#include <sys/un.h>

#include <random>
#include <string>

[[maybe_unused]] static sockaddr_un s;

#define SOCKET_RAND_LEN 0xf
#define MAX_NAME_LENGTH sizeof(s.sun_path) - SOCKET_RAND_LEN - 1

void validateName(const std::string &);
std::string randomString(std::mt19937 &, socklen_t = SOCKET_RAND_LEN);

#endif /* COMMON_HH */
