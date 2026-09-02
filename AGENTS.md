# V Engine  - Repository Knowledge

## What this project is
"V Engine" is a production-oriented, mobile-first **2D game engine + editor** for Android
(phones, tablets, touchscreens). See `README.md` for the full master prompt.

The README explicitly forbids fake functionality and mandates a phase-by-phase
workflow: BUILD → TEST → VALIDATE → FIX → PROFILE → DOCUMENT → COMMIT → NEXT PHASE.

## Current state
- **Phase 01 (Foundation) complete and green.** Platform-independent C++20 core
  builds cleanly and all 26 tests pass.
- The engine directory was renamed `engine/` → `vengine/` so includes use the
  `<vengine/...>` convention. `vengine/Common.hpp` re-exports core type aliases
  (usize, u8, Result, Error, ErrorCode, RecoveryHint, ...) into the `vengine`
  namespace so every module can use them without pulling `core/Types.hpp` directly.
- Android platform layer scaffolded under `android/` (Gradle project + NDK
  `CMakeLists.txt` + JNI entry + Kotlin `GameActivity` + manifest). It is NOT
  compiled in this container (no NDK); only the host core library is.

## Build environment (this container)
- Debian 13 (trixie), GCC 14.2 (full C++20), CMake 3.31, Ninja 1.12.
- No Android SDK/NDK present; APK/AAB builds are scaffolded but not executed here.
- Core engine builds and runs tests via CMake + GoogleTest (fetched by CMake).

## Architecture rules (from README, enforced here)
- C++20, RAII, smart pointers, move semantics, namespaces, const-correctness.
- C++ engine stays platform-independent; Android is a thin platform layer.
- Renderer abstracted (OpenGL ES 3.2 target) so Vulkan can be added later.
- Physics (Box2D), Audio (Oboe) wrapped  - never exposed directly to game devs.
- Atomic file writes for project/scene/asset metadata; meaningful errors.
- Required modules: Core, Platform, Renderer, Input, Physics, Audio, Assets,
  Scene, Entity, Components, UI, Animation, Particles, Editor, Scripting,
  Serialization, Build, Debug, Profiler.

## Module map (under `vengine/` + `android/`)
- `vengine/core`         - logging, assertions, error/result, config, types
- `vengine/math`         - Vec2, Color, Rect, transforms
- `vengine/memory`       - ObjectPool (fixed-capacity, frame-loop allocation free)
- `vengine/scene`        - Scene, Entity (ECS), Registry, view()
- `vengine/components`   - TransformComponent, SpriteRenderer, Rigidbody2D, BoxCollider
- `vengine/serialization`  - JSON value + SceneSerializer (atomic writes)
- `vengine/assets`       - AssetDatabase, dependency tracking
- `vengine/platform`     - IWindow/IFileSystem/IClock + Platform facade struct
- `vengine/input`        - Input API, action/axis mapping, multitouch/pinch
- `vengine/renderer`     - Renderer abstract interface (GLES impl later)
- `vengine/physics`      - Physics abstraction (Box2D backend later)
- `vengine/audio`        - Audio abstraction (Oboe backend later)
- `vengine/ui`           - 2D UI framework
- `vengine/particles`    - particle system
- `vengine/profiler`     - frame profiler
- `vengine/scripting`    - scripting hooks
- `vengine/build`        - build pipeline description
- `vengine/debug`        - debug draw / overlays
- `vengine/Engine.hpp`   - facade: Initialize/Update/Render/Shutdown
- `vengine/Common.hpp`   - core type aliases re-exported into `vengine`
- `android/`             - Gradle project, JNI bridge (JniEntry.cpp), GameActivity,
                          AndroidWindow + AndroidFileSystem platform layer

## Commands
- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` (Ninja auto-detected)
- Build:     `cmake --build build`
- Test:      `ctest --test-dir build --output-on-failure`  (26 tests, all passing)
- Run one:   `./build/bin/vengine_test_scene`

## Conventions discovered
- `Entity` is a 64-bit handle (low 32 = index, high 32 = version). The first
  generation is **1**, never 0, because `raw==0` is the "null/invalid" sentinel.
  `Registry::destroy` bumps the version so stale handles are detected.
- `Registry::view<T...>(fn)` has both const and non-const overloads; the const
  one passes `const T&` to the callback (needed by `SceneSerializer`).
- `ObjectPool` uses a fixed `resize(capacity)` storage + `in_use_` flags so
  free-list indices stay aligned with storage indices (do NOT use
  `emplace_back`, which breaks the index<->slot correspondence).
- `Error::from(...)` takes (code, message) or (code, message, RecoveryHint).
- Results with internal mutexes (Config, AssetDatabase) are non-movable; swap
  the payload under the lock instead of move-assigning the whole object.
- `EXPECT_EQ(a + b, (Vec2f{...}))` breaks the macro on the comma inside the
  parenthesized expression — bind the LHS to a local first.
