#include "util/Config.hpp"

#include <string>

buzzweb::net::ServerConfig LoadConfig() {
    std::optional<std::string> tls_enabled = GetEnvOptional("BUZZWEB_TLS_ENABLED");
    bool tls_required = true;
    if (tls_enabled.has_value()) {
        tls_required = !(*tls_enabled == "0" || *tls_enabled == "FALSE" || *tls_enabled == "false");
    }

    std::optional<std::string> port_str = GetEnvOptional("BUZZWEB_SERVER_PORT");
    unsigned short port = port_str ? static_cast<unsigned short>(std::stoi(*port_str)) : DEFAULT_PORT;

    std::string cert_path;
    std::string private_key;
    if (tls_required) {
        cert_path = GetEnv("BUZZWEB_CERTIFICATE_FILE_PATH");
        private_key = GetEnv("BUZZWEB_PRIVATE_KEY_PATH");
    }

    return {
        port,
        cert_path,
        private_key,
        tls_required
    };
}
