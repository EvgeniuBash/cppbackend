cmake_minimum_required(VERSION 3.11)

project(hello)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(hello
    main.cpp
    http_server.cpp
)

target_link_libraries(hello ${CONAN_LIBS})