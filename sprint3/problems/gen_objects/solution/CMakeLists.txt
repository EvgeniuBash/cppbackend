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
    src/boost_json.cpp
)

target_include_directories(game_server PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(game_server PRIVATE
    CONAN_PKG::boost
    Threads::Threads
)

enable_testing()

add_executable(game_server_tests
    tests/loot_tests.cpp
    src/model.cpp
    src/player.cpp
    src/api_handler.cpp
    src/loot_generator.cpp
    src/json_loader.cpp
    src/request_handler.cpp
    src/logger.cpp
)

target_include_directories(game_server_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(game_server_tests PRIVATE
    CONAN_PKG::catch2
    CONAN_PKG::boost
    Threads::Threads
)

add_test(NAME game_server_tests COMMAND game_server_tests) 
