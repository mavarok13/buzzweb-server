#pragma once

#include "buzzweb/net/Server.hpp"
#include "util/Env.hpp"

constexpr unsigned short DEFAULT_PORT = 9291u;

buzzweb::net::ServerConfig LoadConfig();