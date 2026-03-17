cmake_minimum_required(VERSION 3.11)
project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

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

# Используем Conan targets вместо find_package
target_link_libraries(game_server PRIVATE
    CONAN_PKG::boost_log
    CONAN_PKG::boost_log_setup
    CONAN_PKG::boost_thread
    CONAN_PKG::boost_system
    CONAN_PKG::boost_filesystem
    CONAN_PKG::boost_date_time
    CONAN_PKG::boost_regex
    CONAN_PKG::boost_atomic
    CONAN_PKG::boost_chrono
    Threads::Threads
)
