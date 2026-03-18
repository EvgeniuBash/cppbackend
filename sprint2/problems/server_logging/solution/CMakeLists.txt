cmake_minimum_required(VERSION 3.15)
project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

# Подключаем Conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# Создаем исполняемый файл из всех исходников
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

# Линкуем через Conan
target_link_libraries(game_server PRIVATE
    CONAN_PKG::boost_log
    CONAN_PKG::boost_log_setup
    CONAN_PKG::boost_thread
    CONAN_PKG::boost_system
    CONAN_PKG::boost_filesystem
    CONAN_PKG::boost_json
)