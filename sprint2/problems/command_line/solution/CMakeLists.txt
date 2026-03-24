cmake_minimum_required(VERSION 3.18)
project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/sdk.h
    src/model.h
    src/model.cpp
    src/tagged.h
    src/boost_json.cpp
    src/json_loader.h
    src/json_loader.cpp
    src/request_handler.cpp
    src/request_handler.h
    src/logger.h
    src/logger.cpp
    src/player.h
    src/player.cpp
    src/api_handler.h
    src/api_handler.cpp
)

target_link_libraries(game_server PRIVATE
    Threads::Threads
    CONAN_PKG::boost
)
