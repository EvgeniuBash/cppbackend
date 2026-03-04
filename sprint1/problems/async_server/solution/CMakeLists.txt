cmake_minimum_required(VERSION 3.15)
project(http-server CXX)

if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
    include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif()

find_package(Boost REQUIRED)

add_executable(hello_async main.cpp http_server.cpp)

target_link_libraries(hello_async PRIVATE 
    ${Boost_LIBRARIES} 
    ${CONAN_LIBS}
    pthread
)

target_include_directories(hello_async PRIVATE ${Boost_INCLUDE_DIRS})

target_compile_features(hello_async PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(hello_async PRIVATE /W4)
else()
    target_compile_options(hello_async PRIVATE -Wall -Wextra -pthread)
endif()
