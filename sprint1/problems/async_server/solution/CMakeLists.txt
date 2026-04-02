cmake_minimum_required(VERSION 3.15)
project(hello_async CXX)

if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
    include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()

find_package(Boost REQUIRED)

add_executable(hello_async
    main.cpp
    http_server.cpp
)

target_link_libraries(hello_async PRIVATE
    Boost::boost
    pthread
)

target_compile_features(hello_async PRIVATE cxx_std_17)

set_target_properties(hello_async PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(MSVC)
    target_compile_options(hello_async PRIVATE /W4)
else()
    target_compile_options(hello_async PRIVATE -Wall -Wextra -pthread)
endif()
