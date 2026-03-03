cmake_minimum_required(VERSION 3.15)
project(http-server CXX)

# Указываем путь к заголовочным файлам Boost
set(Boost_INCLUDE_DIR "C:/boost/boost_1_90_0/boost_1_90_0")
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_DEBUG ON)

# Ищем Boost (без компонента system, так как он header-only)
find_package(Boost REQUIRED)

# Добавляем исполняемый файл
add_executable(hello_async main.cpp)

# Добавляем include директории
target_include_directories(hello_async PRIVATE ${Boost_INCLUDE_DIRS})

# Если нужны другие библиотеки (например, thread), добавляем их
# target_link_libraries(hello_async PRIVATE Boost::thread)

# Устанавливаем стандарт C++17
target_compile_features(hello_async PRIVATE cxx_std_17)

# Добавляем флаги компиляции
if(MSVC)
    target_compile_options(hello_async PRIVATE /W4)
else()
    target_compile_options(hello_async PRIVATE -Wall -Wextra)
endif()