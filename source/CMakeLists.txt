cmake_minimum_required(VERSION 3.15)
project(IronNet C)

set(CMAKE_C_STANDARD 99)

file(GLOB SOURCES
        *.h
        *.c
        )

add_executable(IronNet ${SOURCES})