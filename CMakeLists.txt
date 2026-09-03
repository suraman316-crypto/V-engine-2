cmake_minimum_required(VERSION 3.24)

project(VEngine
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "V Engine - mobile-first 2D game engine for Android"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(VENGINE_TESTS "Build V Engine unit tests" ON)
option(VENGINE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# ---- Global compiler options -------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wshadow -Wnon-virtual-dtor -Wno-unused-parameter
    )
    if(VENGINE_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
endif()

# Position-independent code for the shared engine lib used by JNI.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Output libraries/binaries to a single predictable location.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(vengine)
add_subdirectory(sample_game)

if(VENGINE_TESTS)
    enable_testing()
    include(${CMAKE_SOURCE_DIR}/cmake/VEngineTests.cmake)
    add_subdirectory(vengine/tests)
endif()

message(STATUS "V Engine ${PROJECT_VERSION} configured (${CMAKE_BUILD_TYPE})")
