# V Engine test suite.
#
# Each test file is a separate executable so a crash in one cannot mask
# failures in another. All link the vengine static library + gtest_main.

vengine_add_test(vengine_test_core    TestCore.cpp)
vengine_add_test(vengine_test_math    TestMath.cpp)
vengine_add_test(vengine_test_scene   TestScene.cpp)
vengine_add_test(vengine_test_systems TestSystems.cpp)
