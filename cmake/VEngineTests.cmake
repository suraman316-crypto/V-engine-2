# Fetch GoogleTest and make it available to the test target.
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)

# On Windows/MSVC GoogleTest needs this; harmless elsewhere.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Convenience function: register a V Engine test executable with ctest.
function(vengine_add_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE vengine GTest::gtest_main)
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR})
    include(GoogleTest)
    gtest_discover_tests(${name} DISCOVERY_MODE PRE_TEST)
endfunction()
