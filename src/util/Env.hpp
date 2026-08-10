#pragma once

#include <string>
#include <optional>

std::string GetEnv(const std::string & env);
std::optional<std::string> GetEnvOptional(const std::string & env);