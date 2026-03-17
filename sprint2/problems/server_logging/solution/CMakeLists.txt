cmake_minimum_required(VERSION 3.11)
project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

# Подключаем Conan 1
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Потоки
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Исполняемый файл
add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/model.cpp
    src/boost_json.cpp
    src/json_loader.cpp
    src/request_handler.cpp
    src/logger.cpp
)

# Линкуем все через CONAN_LIBS + зависимости Boost.Log + Threads
target_link_libraries(game_server
    ${CONAN_LIBS}
    Threads::Threads
    dl      # для Linux
)
