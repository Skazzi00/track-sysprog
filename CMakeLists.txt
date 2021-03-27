cmake_minimum_required(VERSION 3.17)
project(track_sysprog C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-D_XOPEN_SOURCE -fsanitize=address -g")

include_directories(coro)

add_executable(track_sysprog coro/main.c coro/coro.c)

