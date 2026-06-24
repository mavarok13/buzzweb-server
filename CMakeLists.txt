cmake_minimum_required(VERSION 3.24)

project(
    BuzzWebServer
    VERSION 0.1.0
    DESCRIPTION "Self-hosted voice/video signaling server"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED CONFIG COMPONENTS system log log_setup thread)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(buzzweb_server INTERFACE)
add_library(BuzzWeb::Server ALIAS buzzweb_server)

target_compile_features(buzzweb_server INTERFACE cxx_std_20)

target_include_directories(
    buzzweb_server
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(
    buzzweb_server
    INTERFACE
        Boost::system
        Boost::log
        Boost::log_setup
        Boost::thread
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
)
