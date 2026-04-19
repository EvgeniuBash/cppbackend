cmake_minimum_required(VERSION 3.15)
project(book_manager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_executable(book_manager main.cpp)

target_link_libraries(book_manager CONAN_PKG::libpqxx CONAN_PKG::nlohmann_json)

# 👇 после conan!
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})