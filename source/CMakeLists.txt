cmake_minimum_required(VERSION 3.15)
project(IronNet C)

set(CMAKE_C_STANDARD 99)

add_executable(IronNet
        main.c
        appDomain.h
        cliFile.h
        cliFile.c
        ilOpcode.h
        irOpcode.h
        irStructures.h
        metadataStructures.h
        metadataStructures.c
        uthash.h)