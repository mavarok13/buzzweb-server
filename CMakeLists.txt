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

add_library(
    buzzweb_server_lib
    STATIC
        src/app/ControlDispatcher.cpp
        src/app/RoomService.cpp
        src/domain/InMemoryRoomRepository.cpp
        src/domain/Participant.cpp
        src/domain/Room.cpp
        src/net/Listener.cpp
        src/net/Server.cpp
        src/net/Session.cpp
        src/net/SessionRegistry.cpp
)
add_library(BuzzWeb::Server ALIAS buzzweb_server_lib)

target_compile_features(buzzweb_server_lib PUBLIC cxx_std_20)

target_include_directories(
    buzzweb_server_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(
    buzzweb_server_lib
    PUBLIC
        Boost::system
        Boost::log
        Boost::log_setup
        Boost::thread
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
)

add_executable(
    buzzweb_server
        src/main.cpp
        src/util/Env.cpp
        src/util/Config.cpp
)

target_include_directories(
    buzzweb_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src    
)

target_link_libraries(
    buzzweb_server
    PRIVATE
        buzzweb_server_lib
)
