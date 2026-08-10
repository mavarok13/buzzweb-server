#include "util/Env.hpp"

#include <cstdlib>
#include <stdexcept>

std::string GetEnv(const std::string & env) {
    if (const char * val = std::getenv(env.c_str())) {
        return val;
    }

    throw std::runtime_error("getenv_error");
} 

std::optional<std::string> GetEnvOptional(const std::string & env) {
    if (const char * val = std::getenv(env.c_str())) {
        return val;
    }

    return std::nullopt;
}