# V Engine core library.
#
# A single static library "vengine" aggregates the platform-independent C++ core.
# The Android platform layer (under android/) links against this library via JNI.

set(VENGINE_SOURCES
    core/Logging.cpp
    core/Error.cpp
    core/Assert.cpp
    core/Config.cpp
    math/Vec2.cpp
    math/Color.cpp
    memory/ObjectPool.cpp
    scene/Scene.cpp
    components/Components.cpp
    serialization/Serialization.cpp
    serialization/SceneSerializer.cpp
    assets/AssetDatabase.cpp
    platform/Platform.cpp
    input/Input.cpp
    ui/UI.cpp
    particles/Particles.cpp
    profiler/Profiler.cpp
    Engine.cpp
)

add_library(vengine STATIC ${VENGINE_SOURCES})

target_include_directories(vengine PUBLIC ${CMAKE_SOURCE_DIR})

# The core engine has no third-party runtime dependencies by design.
target_compile_definitions(vengine PUBLIC
    VENGINE_VERSION="${PROJECT_VERSION}"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(vengine PRIVATE -fvisibility=hidden)
endif()
