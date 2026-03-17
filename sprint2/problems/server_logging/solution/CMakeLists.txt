cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

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

# 🔥 обязательно для Boost.Log
target_compile_definitions(game_server PRIVATE
    BOOST_LOG_DYN_LINK
)

# ✅ ВОТ КЛЮЧЕВОЙ ФИКС
target_link_libraries(game_server
    ${CONAN_LIBS}

    boost_log
    boost_log_setup

    Threads::Threads
    dl
)