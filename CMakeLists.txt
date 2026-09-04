cmake_minimum_required(VERSION 3.20)

project(
    IronNet
    VERSION 0.2.0
    DESCRIPTION "Portable ECMA-335 interpreter"
    LANGUAGES C
)

include(CTest)
include(GNUInstallDirs)

option(IRON_BUILD_SHARED "Build IronNet as a shared library" OFF)
option(IRON_BUILD_CLI "Build the IronNet command-line host" ON)
option(IRON_BUILD_TESTS "Build the native test suite" ${BUILD_TESTING})
option(IRON_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(IronWarnings)

set(IRON_SOURCES
    src/core/platform.c
    src/core/memory.c
    src/core/types.c
    src/core/debug.c
    src/pe/pe.c
    src/metadata/metadata.c
    src/runtime/opcode_table.c
    src/runtime/exec.c
    src/runtime/runtime.c
    src/runtime/monitor.c
    src/thread/thread.c
    src/gc/gc.c
    src/corlib/value.c
    src/corlib/string.c
    src/corlib/object.c
    src/corlib/valuetype.c
    src/corlib/console.c
    src/corlib/math.c
    src/corlib/int32.c
    src/corlib/floating.c
    src/corlib/environment.c
    src/corlib/array.c
    src/corlib/gc.c
    src/corlib/type.c
    src/corlib/reflection.c
    src/corlib/enum.c
    src/corlib/runtime_helpers.c
    src/corlib/threading.c
    src/corlib/marshal.c
    src/corlib/corlib_main.c
    src/disasm/disasm.c
    src/iron.c
)

if(WIN32)
    list(APPEND IRON_SOURCES
        src/platform/platform_windows.c
        src/thread/thread_windows.c
    )
elseif(UNIX)
    list(APPEND IRON_SOURCES
        src/platform/platform_posix.c
        src/thread/thread_posix.c
    )
else()
    message(FATAL_ERROR "IronNet requires a Windows or POSIX threading backend")
endif()

if(IRON_BUILD_SHARED)
    add_library(iron SHARED ${IRON_SOURCES})
    target_compile_definitions(iron PRIVATE IRON_BUILD_SHARED)
else()
    add_library(iron STATIC ${IRON_SOURCES})
endif()

add_library(IronNet::iron ALIAS iron)

set_target_properties(iron PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE YES
)

target_include_directories(iron
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

iron_enable_strict_warnings(iron)

find_package(Threads REQUIRED)
target_link_libraries(iron PUBLIC Threads::Threads)

if(NOT WIN32)
    target_link_libraries(iron PUBLIC m)
endif()

if(IRON_BUILD_CLI)
    add_executable(ironnet src/main.c)
    set_target_properties(ironnet PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
    )
    target_link_libraries(ironnet PRIVATE IronNet::iron)
    iron_enable_strict_warnings(ironnet)
endif()

if(IRON_BUILD_TESTS)
    add_executable(iron_tests tests/test_main.c)
    set_target_properties(iron_tests PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
    )
    target_link_libraries(iron_tests PRIVATE IronNet::iron)
    iron_enable_strict_warnings(iron_tests)
    add_test(NAME iron.unit COMMAND iron_tests)
endif()

install(
    TARGETS iron
    EXPORT IronNetTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(TARGET ironnet)
    install(TARGETS ironnet RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(DIRECTORY include/iron DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(
    EXPORT IronNetTargets
    FILE IronNetTargets.cmake
    NAMESPACE IronNet::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IronNet
)
