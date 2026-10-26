cmake_minimum_required(VERSION 3.16)
project(algoflow C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(raylib 4.0 REQUIRED)
add_executable(algoflow main.c)
target_link_libraries(algoflow PRIVATE raylib)

if (UNIX AND NOT APPLE)
  target_link_libraries(algoflow PRIVATE m)
endif()
