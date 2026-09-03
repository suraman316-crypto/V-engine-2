# V Engine test suite.
#
# Each test file is a separate executable so a crash in one cannot mask
# failures in another. All link the vengine static library + gtest_main.

vengine_add_test(vengine_test_core    TestCore.cpp)
vengine_add_test(vengine_test_math    TestMath.cpp)
vengine_add_test(vengine_test_scene   TestScene.cpp)
vengine_add_test(vengine_test_systems TestSystems.cpp)
vengine_add_test(vengine_test_systems2 TestSystems2.cpp)
vengine_add_test(vengine_test_physics  TestPhysics.cpp)
vengine_add_test(vengine_test_engine2  TestEngine2.cpp)
vengine_add_test(vengine_test_anim     TestAnimScript.cpp)

# V2 module tests (physics v2, renderer v2, systems v2, engine v2 facade).
vengine_add_test(vengine_test_physics_v2  TestPhysicsV2.cpp)
vengine_add_test(vengine_test_renderer_v2 TestRendererV2.cpp)
vengine_add_test(vengine_test_systems_v2  TestSystemsV2.cpp)
vengine_add_test(vengine_test_engine_v2   TestEngineV2.cpp)
vengine_add_test(vengine_test_phase19     TestPhase19.cpp)
vengine_add_test(vengine_test_v2extras    TestV2Extras.cpp)
vengine_add_test(vengine_test_v2utility   TestV2Utility.cpp)
