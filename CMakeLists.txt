cmake_minimum_required(VERSION 3.17)
project(track_sysprog C)

set(CMAKE_C_STANDARD 11)

include_directories(coro)

add_executable(track_sysprog
        coro/main.c
)
