cmake_minimum_required(VERSION 3.16)

project(NobetUzunKoridor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(nightshift src/main.cpp)
add_executable(nightshift-server src/server.cpp)

if(MSVC)
    target_compile_options(nightshift PRIVATE /W4)
    target_compile_options(nightshift-server PRIVATE /W4)
else()
    target_compile_options(nightshift PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(nightshift-server PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)
target_link_libraries(nightshift-server PRIVATE Threads::Threads)

if(WIN32)
    target_link_libraries(nightshift-server PRIVATE ws2_32)
endif()
