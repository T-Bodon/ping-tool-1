cmake_minimum_required(VERSION 3.14)
project(ping)

set(CMAKE_CXX_STANDARD 17)

add_executable(ping main.cpp utils.h utils.cpp ping_error.h pinger.h)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ggdb -std=c++17 -pedantic -O2 -flto")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG -fsanitize=address,undefined,leak")
endif ()