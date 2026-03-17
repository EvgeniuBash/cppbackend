cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/model.cpp
    src/boost_json.cpp
    src/json_loader.cpp
    src/request_handler.cpp
    src/logger.cpp
)

target_compile_definitions(game_server PRIVATE BOOST_LOG_DYN_LINK)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost 1.78.0 REQUIRED COMPONENTS
    log
    log_setup
    thread
    system
    filesystem
    date_time
    regex
    chrono
    atomic
)
if(Boost_FOUND)
    target_include_directories(game_server PRIVATE ${Boost_INCLUDE_DIRS})
endif()

target_link_libraries(game_server
    PRIVATE
    Threads::Threads
    Boost::log
    Boost::log_setup
    Boost::thread
    Boost::system
    Boost::filesystem
    Boost::date_time
    Boost::regex
    Boost::chrono
    Boost::atomic
)
