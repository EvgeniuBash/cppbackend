cmake_minimum_required(VERSION 3.18)
project(game_server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(game_lib STATIC
    src/model.cpp
    src/json_loader.cpp
    src/player.cpp
    src/loot_generator.cpp
    src/logger.cpp
)

target_include_directories(game_lib PUBLIC
    src
)

target_link_libraries(game_lib PUBLIC
    CONAN_PKG::boost
    Threads::Threads
)

add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/request_handler.cpp
    src/api_handler.cpp
)

target_link_libraries(game_server
    PRIVATE game_lib
)

add_executable(game_server_tests
    tests/loot_generator_tests.cpp
    tests/loot_tests.cpp
)

target_link_libraries(game_server_tests
    PRIVATE
    game_lib
    CONAN_PKG::catch2
    CONAN_PKG::catch2_main
)

include(CTest)
enable_testing()
add_test(NAME game_server_tests COMMAND game_server_tests)