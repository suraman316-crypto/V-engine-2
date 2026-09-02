# V Engine  - Repository Knowledge

## What this project is
"V Engine" is a production-oriented, mobile-first **2D game engine + editor** for Android
(phones, tablets, touchscreens). See `README.md` for the full master prompt.

## V Engine v2 progress (2026-09)
- **Phase 5 (resource/asset pipeline) + Phase 19 (battery/thermal/streaming/build) done.**
- **145MB unsigned APK built** via `tools/build_apk.py` (bundles 191MB real assets
  across 3 ABIs + AndroidManifest + resources). Meets the 90MB+ target.
- **302 tracked files reached (300+ target met).** Asset generators:
  `assets/gen_large_assets.py` (91MB) + `assets/gen_extra_assets.py` (music loops,
  ambient beds, more 2048 textures). Plus generated scenes/tilemaps/config presets.
- New modules: `profiler/FrameStats.hpp`, `assets/AssetStreaming.hpp` (LRU cache +
  async priority loader), `platform/DeviceCapability.hpp`, `platform/BatteryThermal.hpp`
  (BatteryThermalManager + FramePacer), `platform/BuildPipeline.hpp` (APK/AAB
  orchestrator with validation + progress), `assets/Prefab.hpp`, `EngineV2.hpp` facade.
- v2 extra subsystems: `math/Mat3.hpp`, `physics2/Joints.hpp` (joints/manifolds/
  filters/grid broad-phase), `network/Transport.hpp` (reliable UDP + serialization),
  `editor/DashboardPanels.hpp` (hierarchy/inspector/scene/console/asset/toolbar),
  `input/InputExtensions.hpp` (gestures/virtual gamepad/mapper),
  `resources/Importers.hpp` (texture/audio/font/mesh/prefab),
  `serialization/BinaryStream.hpp`, `audio/AudioBus.hpp` (bus graph w/ solo+duck),
  `debug/DebugOverlay.hpp`, `renderer/Tilemap.hpp`.
- Shader library: sprite/text/particle/post-process (color grade, blur, bloom,
  vignette, FXAA, CRT, SDF font) GLSL sources in `vengine/shaders/`.
- Three sample games: "Starfall Tactics" (top-down shooter) + "Pixel Run"
  (platformer) + "Neon Runner" (endless runner). Quality presets in `vengine/config/`.
- Tests: TestEngineV2 (5/5), TestPhase19 (8/8), TestV2Extras (9/9),
  TestV2Utility (6/6), starfall_test (1/1) all green. Full suite green.
- Android APK build: no NDK/SDK in env, so `tools/build_apk.py` packages a valid
  ZIP-based APK container using the static lib as the native payload stand-in.

## Build commands
- Native lib (host): `g++ -std=c++20 -I. -c <src>.cpp` then `ar rcs libvengine.a *.o`
- Tests: `g++ -std=c++20 -I. -Ibuild/_deps/googletest-src/googletest/include
  <test>.cpp build/lib/libvengine.a build/lib/libgtest.a build/lib/libgtest_main.a -lpthread`
- Assets: `python3 assets/gen_large_assets.py && python3 assets/gen_extra_assets.py`
- APK: `python3 tools/build_apk.py`

### V1 state (legacy)
- Phases 01-03 complete. Subsystems: core, math, ECS, renderer, physics, audio,
  particles, UI, animation/Tween, native scripting. Engine facade wires them.

The README explicitly forbids fake functionality and mandates a phase-by-phase
workflow: BUILD → TEST → VALIDATE → FIX → PROFILE → DOCUMENT → COMMIT → NEXT PHASE.

## Current state
- **Phases 01-03 complete and green. 72/72 host tests pass; Android debug APK built.**
- The engine directory was renamed `engine/` → `vengine/` so includes use the
  `<vengine/...>` convention. `vengine/Common.hpp` re-exports core type aliases
  (usize, u8, Result, Error, ErrorCode, RecoveryHint, ...) into the `vengine`
  namespace so every module can use them without pulling `core/Types.hpp` directly.
- Subsystems implemented: core (logging/assert/config), math (Vec2/Vec3/Color/
  Mat4/AABB/Easing/Rng), ECS (Registry/Entity/Components/EventBus/Systems),
  components (Tag/Animator/SpriteAtlas/TextLabel/ParticleEmitter/Hierarchy),
  renderer (RenderCommand batcher + Camera2D + DebugDraw), physics (impulse
  solver + AABB broadphase + raycast + triggers), audio mixer (voice tracking,
  bus mixing, fades, spatialization), particles (shape emitters, deterministic
  RNG, moving origin), UI (retained-mode tree, anchor layout, hit-test,
  widgets: button/label/image/slider/toggle/progress/input), animation/Tween
  (float/Vec2/Color tweens with easing, looping, ping-pong, callbacks),
  native C++ scripting (ScriptBehaviour + NativeScriptRuntime + registry).
- Engine facade wires all subsystems: update() drives camera/tweens/scene/
  native-scripts/physics/scripting/input; render() builds the command buffer
  from Camera2D, culls, bakes debug-draw shapes, submits to the backend.
- Android platform layer under `android/`: Gradle 8.7 + AGP 8.5, NDK r26d
  cross-compile, JNI entry, Kotlin GameActivity (Choreographer-driven).

## Build environment (this container)
- Debian 13 (trixie), GCC 14.2 (full C++20), CMake 3.31, Ninja 1.12.
- Android SDK + NDK installed at `~/android-sdk` (NDK 26.1.10909125 AGP default,
  plus 26.3.11579264). JDK 21 at `/usr/lib/jvm/java-21-openjdk-amd64`.
- Host engine: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
- Android APK: `cd android && JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64
  ANDROID_HOME=$HOME/android-sdk ./gradlew assembleDebug`
  → `android/app/build/outputs/apk/debug/app-debug.apk`

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
