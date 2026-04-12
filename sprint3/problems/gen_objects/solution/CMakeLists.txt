cmake_minimum_required(VERSION 3.18)
project(game_server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/model.cpp
    src/json_loader.cpp
    src/request_handler.cpp
    src/logger.cpp
    src/player.cpp
    src/api_handler.cpp
    src/loot_generator.cpp
)

target_include_directories(game_server PRIVATE CONAN_PKG::boost)

target_link_libraries(game_server
    PRIVATE CONAN_PKG::boost
    Threads::Threads
)

add_executable(game_server_tests
    src/model.cpp
    src/player.cpp
    src/loot_generator.cpp

    tests/loot_generator_tests.cpp
    tests/loot_tests.cpp
)

target_include_directories(game_server_tests
    PRIVATE
    src
    CONAN_PKG::boost
)

target_link_libraries(game_server_tests
    PRIVATE CONAN_PKG::catch2
    CONAN_PKG::boost
    Threads::Threads
)