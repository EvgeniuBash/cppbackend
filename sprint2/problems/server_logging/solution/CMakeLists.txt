cmake_minimum_required(VERSION 3.23)
project(game_server LANGUAGES CXX)

include(${CMAKE_TOOLCHAIN_FILE})

find_package(boost REQUIRED)

add_executable(game_server
    src/main.cpp
)

target_link_libraries(game_server PRIVATE boost::boost)
