# V Engine 2.0 — sample game: "Starfall Tactics".
#
# A thin gameplay library that links against the engine. On Android it is
# linked into the same libvengine.so; on desktop it is a standalone test
# binary so the sample can be run headless.

add_library(starfall STATIC
    scripts/GameBehaviours.cpp
    platformer/PlatformerGame.cpp
    runner/RunnerGame.cpp
)
target_include_directories(starfall PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(starfall PUBLIC vengine)

if(VENGINE_TESTS)
    add_executable(starfall_test scripts/main.cpp)
    target_link_libraries(starfall_test PRIVATE starfall GTest::gtest_main)
    target_include_directories(starfall_test PRIVATE ${CMAKE_SOURCE_DIR})
endif()
