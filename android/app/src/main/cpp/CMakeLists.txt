# V Engine Android native build.
#
# Builds libvengine.so from the shared engine sources plus the Android
# platform layer and JNI entry. The host engine library is compiled directly
# into the .so (no separate static lib on-device) to keep the symbol surface
# minimal.

cmake_minimum_required(VERSION 3.31.0)
project(vengine_android LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Engine sources (mirrors vengine/CMakeLists.txt minus the Android platform
# layer, which is supplied here).
set(VENGINE_SOURCES
    ../../../../vengine/core/Assert.cpp
    ../../../../vengine/core/Config.cpp
    ../../../../vengine/core/Error.cpp
    ../../../../vengine/core/Logging.cpp
    ../../../../vengine/math/Color.cpp
    ../../../../vengine/math/Vec2.cpp
    ../../../../vengine/memory/ObjectPool.cpp
    ../../../../vengine/scene/Scene.cpp
    ../../../../vengine/components/Components.cpp
    ../../../../vengine/serialization/Serialization.cpp
    ../../../../vengine/serialization/SceneSerializer.cpp
    ../../../../vengine/assets/AssetDatabase.cpp
    ../../../../vengine/platform/Platform.cpp
    ../../../../vengine/input/Input.cpp
    ../../../../vengine/ui/UI.cpp
    ../../../../vengine/particles/Particles.cpp
    ../../../../vengine/profiler/Profiler.cpp
    ../../../../vengine/Engine.cpp
    AndroidPlatform.cpp
    JniEntry.cpp
)

add_library(vengine SHARED ${VENGINE_SOURCES})

target_include_directories(vengine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(vengine PRIVATE ${log-lib} ${android-lib})
