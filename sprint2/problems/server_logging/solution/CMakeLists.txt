cmake_minimum_required(VERSION 3.16)
project(game_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS log log_setup thread system filesystem)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found!")
endif()

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/logger.cpp
    # Добавьте сюда другие .cpp файлы
)

add_executable(game_server ${SOURCES})

target_include_directories(game_server PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(game_server PRIVATE
    ${Boost_LIBRARIES}    
    pthread      
)
