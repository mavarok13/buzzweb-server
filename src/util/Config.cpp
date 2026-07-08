#include "util/Config.hpp"

#include <string>

buzzweb::net::ServerConfig LoadConfig() {
    std::string cert_path = GetEnv("BUZZWEB_CERTIFICATE_FILE_PATH");
    std::string private_key = GetEnv("BUZZWEB_PRIVATE_KEY_PATH");

    std::optional<std::string> port_str = GetEnvOptional("BUZZWEB_SERVER_PORT");
    unsigned short port = port_str ? static_cast<unsigned short>(std::stoi(*port_str)) : DEFAULT_PORT;

    return {port, cert_path, private_key};
}