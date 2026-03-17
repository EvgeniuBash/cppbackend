cmake_minimum_required(VERSION 3.18)
project(game_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(boost REQUIRED COMPONENTS log log_setup thread system filesystem)

file(GLOB SOURCES src/*.cpp src/*.h)

add_executable(game_server ${SOURCES})

target_link_libraries(game_server PRIVATE Boost::log Boost::log_setup Boost::thread Boost::system Boost::filesystem)
