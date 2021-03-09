cmake_minimum_required(VERSION 3.16)
project(Dna C)

set(CMAKE_C_STANDARD 99)

file(GLOB SOURCES
        *.h
        *.c
        )

IF(UNIX)
    set(CMAKE_C_FLAGS "-m32")
ELSE(WIN32)
    set(CMAKE_C_FLAGS "/D_UNICODE /DUNICODE")
ENDIF()

add_executable(Dna ${SOURCES})

IF(UNIX)
    target_link_libraries(Dna m dl pthread)
ELSE(WIN32)
    target_link_libraries(Dna user32 wsock32 ws2_32)
ENDIF()
