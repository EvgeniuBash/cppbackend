cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

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
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(SOURCES
    src/main.cpp
    src/http_server.cpp
    src/model.cpp
    src/boost_json.cpp
    src/json_loader.cpp
    src/request_handler.cpp
    src/logger.cpp
)

set(HEADERS
    src/http_server.h
    src/sdk.h
    src/model.h
    src/tagged.h
    src/json_loader.h
    src/request_handler.h
    src/logger.h
)

add_executable(game_server ${SOURCES} ${HEADERS})

target_link_libraries(game_server
    PRIVATE
        ${Boost_LIBRARIES}
        Threads::Threads
)
