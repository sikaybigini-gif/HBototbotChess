cmake_minimum_required(VERSION 3.16)

project(NobetUzunKoridor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(nightshift src/main.cpp)

if(MSVC)
    target_compile_options(nightshift PRIVATE /W4)
else()
    target_compile_options(nightshift PRIVATE -Wall -Wextra -Wpedantic)
endif()
