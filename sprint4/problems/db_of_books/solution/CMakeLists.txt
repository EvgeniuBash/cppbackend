cmake_minimum_required(VERSION 3.15)
project(book_manager)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(book_manager main.cpp)

target_link_libraries(book_manager ${CONAN_LIBS})