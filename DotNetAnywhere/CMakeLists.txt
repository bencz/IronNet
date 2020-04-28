cmake_minimum_required(VERSION 3.16)
project(Dna C)

set(CMAKE_C_STANDARD 99)

file(GLOB SOURCES
        *.h
        *.c
        )

set(CMAKE_C_FLAGS "-m32")
add_executable(Dna ${SOURCES})
IF(UNIX)
    target_link_libraries(Dna m dl pthread)
ENDIF()
