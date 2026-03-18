cmake_minimum_required(VERSION 3.11)
project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Находим Boost через Conan
find_package(Boost REQUIRED COMPONENTS log log_setup thread system filesystem json)

add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/http_server.h
    src/sdk.h
    src/model.h
    src/model.cpp
    src/tagged.h
    src/boost_json.cpp
    src/json_loader.h
    src/json_loader.cpp
    src/request_handler.cpp
    src/request_handler.h
    src/logger.cpp
    src/logger.h
)

target_link_libraries(game_server
    PRIVATE
    Boost::log
    Boost::log_setup
    Boost::thread
    Boost::system
    Boost::filesystem
    Boost::json
    Threads::Threads
    dl
)