# V-engine-2
android 2D game engine 
this project for android mobile


MASTER PROMPT — FULL 2D ANDROID APP MOBILE GAME ENGINE

PROJECT NAME

Build a complete, production-oriented, mobile-first 2D game engine named:

[ V Engine ]

The engine must focus exclusively on 2D game development and must provide a workflow inspired by professional engines such as Unity, but it must have its own architecture, UI, APIs, file formats, runtime, editor, and toolchain.

The target platforms are:

- Android phones android app
- Android tablets
- Touchscreen devices
- Different screen sizes and aspect ratios
- Low-end, mid-range, and high-end Android devices

The engine must be designed as a real software product, not a demo, mockup, prototype, or collection of disconnected examples.

---

ABSOLUTE DEVELOPMENT RULES

1. Do not create fake functionality.
2. Do not create buttons that do nothing.
3. Do not use placeholder implementations for core functionality.
4. Every visible tool must either work or clearly report why it cannot execute.
5. Every subsystem must have defined interfaces.
6. Every subsystem must have error handling.
7. Every subsystem must have validation.
8. Every important subsystem must have automated tests.
9. Do not silently ignore errors.
10. Do not corrupt project files when an operation fails.
11. Use atomic file writes for project/scene/asset metadata.
12. Preserve user data whenever possible.
13. Never overwrite an asset without explicit user action.
14. Support undo/redo for editor operations wherever technically appropriate.
15. Save operations must be recoverable.
16. Prevent crashes caused by malformed assets.
17. Prevent crashes caused by invalid scenes.
18. Prevent crashes caused by missing files.
19. Provide meaningful error messages.
20. Optimize for mobile hardware.
21. Avoid unnecessary memory allocations inside the frame loop.
22. Avoid blocking the UI thread during expensive operations.
23. Use background workers for asset importing and expensive processing where possible.
24. Provide cancellation for long-running operations.
25. Provide progress indicators for long operations.
26. Never claim a feature is complete until it has been implemented and tested.
27. Maintain backward compatibility for project files whenever practical.
28. Keep the architecture modular.
29. Avoid unnecessary dependencies.
30. Prefer mature, documented, actively maintained libraries/APIs.

---

RECOMMENDED TECHNOLOGY STACK

Use the following technologies unless there is a technically justified reason to replace one.

Core Language

C++20 or newer supported C++ standard

Use modern C++ practices:

- RAII
- smart pointers where appropriate
- move semantics
- strong types
- namespaces
- templates where useful
- clear ownership rules
- const correctness
- thread safety where required

Avoid unnecessary raw ownership pointers.

---

ANDROID

Use:

- Android SDK
- Android NDK
- CMake
- Gradle
- Kotlin/Java only where Android platform integration requires it
- JNI for communication between Kotlin/Java and C++

The C++ engine must remain as platform-independent as reasonably possible.

Create a clean Android platform layer.

---

GRAPHICS

Primary rendering API:

OpenGL ES 3.2

Design the renderer behind an abstraction layer so that Vulkan can be added later.

Renderer responsibilities:

- 2D sprites
- sprite batching
- textures
- texture atlases
- UV coordinates
- spritesheets
- nine-slice rendering
- text rendering
- shapes
- lines
- circles
- rectangles
- polygons
- render targets
- framebuffers
- cameras
- layers
- sorting
- blending
- masking
- shaders
- materials
- render passes
- resolution scaling
- dynamic resolution where appropriate

The renderer must be optimized for mobile GPUs.

---

WINDOW AND INPUT

Use a suitable platform abstraction such as:

SDL3 or GLFW where compatible with the Android architecture

The engine must provide its own Input API.

Support:

- touch
- multitouch
- gestures
- mouse when available
- keyboard when available
- gamepads/controllers
- virtual controls
- screen rotation
- safe areas
- different DPI values

Input API must support:

- action mapping
- axis mapping
- touch coordinates
- normalized coordinates
- world coordinates
- UI coordinates
- gesture events

---

PHYSICS

Use:

Box2D

Create an engine-level physics abstraction above Box2D.

Support:

- Rigidbody2D
- Static bodies
- Dynamic bodies
- Kinematic bodies
- Box collider
- Circle collider
- Capsule collider where supported by abstraction
- Polygon collider
- Edge collider
- Sensors/triggers
- Collision layers
- Collision masks
- Physics materials
- Gravity
- Forces
- Torque
- Impulses
- Raycasts
- Shape casts where supported
- Collision callbacks
- Trigger callbacks

Do not expose the external physics library directly to game developers.

---

AUDIO

Use a suitable mobile-friendly audio backend such as:

Android Oboe

Provide an engine-level audio system.

Support:

- sound effects
- music
- looping
- volume
- pitch
- stereo
- audio buses
- master bus
- SFX bus
- music bus
- UI bus
- mute
- pause
- resume
- audio fading
- spatialized 2D audio
- streaming music
- compressed audio assets

---

UI

Create a complete 2D UI framework.

Support:

- panels
- buttons
- labels
- images
- sliders
- progress bars
- toggles
- checkboxes
- dropdowns
- scroll views
- lists
- grids
- input fields
- popups
- dialogs
- tabs
- navigation
- anchors
- pivots
- margins
- padding
- responsive layouts
- DPI scaling
- localization-ready text
- touch-friendly controls

UI must work correctly on phones and tablets.

---

20-PHASE DEVELOPMENT PLAN

PHASE 01 — FOUNDATION

Create:

- repository
- CMake configuration
- Android project
- NDK integration
- build configurations
- Debug build
- Release build
- logging system
- assertions
- error system
- configuration system
- platform abstraction
- engine lifecycle

Create clean module boundaries.

Required modules:

Core
Platform
Renderer
Input
Physics
Audio
Assets
Scene
Entity
Components
UI
Animation
Particles
Editor
Scripting
Serialization
Build
Debug
Profiler

---

PHASE 02 — ANDROID PLATFORM LAYER

Implement:

- Android lifecycle
- Activity integration
- Surface creation/destruction
- pause/resume
- app backgrounding
- app foregrounding
- orientation
- screen size detection
- density detection
- safe-area handling
- touch input
- file access
- permissions
- storage abstraction

Handle:

- device rotation
- activity recreation
- renderer recreation
- context loss
- low-memory situations

Never assume the device has a fixed resolution.

---

PHASE 03 — 2D RENDERER

Build the complete renderer.

Implement:

- OpenGL ES initialization
- shaders
- shader compilation errors
- texture loading
- texture creation
- sprite rendering
- batching
- sorting
- camera
- viewport
- render targets
- framebuffer management
- blending
- alpha
- color modulation
- sprite flipping
- clipping
- masking

Optimize:

- draw calls
- texture switches
- state changes
- allocations
- overdraw

---

PHASE 04 — ASSET SYSTEM

Create a complete asset pipeline.

Supported assets:

- PNG
- JPG/JPEG
- WebP
- SVG where appropriate
- texture atlases
- sprite sheets
- fonts
- audio
- JSON
- engine scenes
- prefabs
- materials
- shaders
- animations
- tilemaps
- particle configurations

Create:

Asset Database

Each asset must have:

- unique identifier
- source path
- imported path
- type
- metadata
- dependencies
- importer settings
- modification timestamp
- version information

Implement:

- import
- reimport
- delete
- rename
- move
- duplicate
- dependency tracking
- missing asset detection
- corrupted asset detection

---

PHASE 05 — PHONE/TABLET ASSET IMPORT

The editor must allow users to add assets directly from an Android phone or tablet.

Support:

- Android file picker
- system document picker
- Photos/Gallery where permissions allow
- multiple file selection
- folder selection where Android permits it
- drag-and-drop where supported
- copy/import option
- reference/link option where technically safe

When an asset is imported:

1. Validate it.
2. Copy it into the project if requested.
3. Generate metadata.
4. Import/process it.
5. Generate thumbnails.
6. Add it to the Asset Database.
7. Make it immediately available to the editor.

Never silently lose the original asset.

Support:

- progress
- cancellation
- duplicate detection
- unsupported-file warnings
- corrupted-file warnings
- insufficient-storage warnings

---

PHASE 06 — SCENE SYSTEM

Create a real scene system.

Support:

- scenes
- entities
- components
- hierarchy
- parent/child relationships
- transforms
- activation/deactivation
- layers
- tags
- scene loading
- scene unloading
- scene transitions

Provide serialization.

Scene files must be versioned.

Example conceptual structure:

Project
 ├── Scenes
 │    ├── Main.scene
 │    ├── Menu.scene
 │    └── Level01.scene
 ├── Assets
 ├── Prefabs
 ├── Materials
 ├── Audio
 └── Scripts

---

PHASE 07 — ENTITY COMPONENT SYSTEM

Create a robust component architecture.

Built-in components:

- Transform2D
- SpriteRenderer
- Camera2D
- Rigidbody2D
- Collider2D
- Animator
- ParticleEmitter
- AudioSource
- AudioListener
- Canvas
- UI components
- Tilemap
- Script component

Support:

- component creation
- deletion
- serialization
- cloning
- runtime creation
- runtime destruction
- enable/disable
- dependency validation

---

PHASE 08 — 2D PHYSICS

Integrate Box2D.

Implement all required engine-level APIs.

Add editor visualization for:

- colliders
- contact points
- velocity
- center of mass
- joints
- raycasts

Support:

- physics layers
- collision matrix
- triggers
- callbacks
- queries

Provide deterministic behavior as far as practical.

---

PHASE 09 — SPRITE + ANIMATION

Create professional 2D animation tools.

Support:

- spritesheets
- frame animation
- animation clips
- animation timelines
- keyframes
- easing
- interpolation
- animation events
- state machines
- transitions
- parameters
- blend logic where appropriate
- animation preview

Provide an Animator editor.

---

PHASE 10 — TILEMAP SYSTEM

Create a complete tilemap system.

Support:

- tiles
- tile palettes
- tilemap layers
- grid
- autotiling
- terrain rules
- collision generation
- tile animation
- brushes
- rectangle tools
- line tools
- fill tools
- erase tools
- selection
- copy/paste
- undo/redo

Support large maps efficiently.

---

PHASE 11 — PARTICLE SYSTEM

Create a 2D particle system.

Support:

- emission
- lifetime
- velocity
- acceleration
- gravity
- size
- rotation
- color
- alpha
- texture
- sprite animation
- bursts
- randomization
- trails where practical
- local/world simulation
- particle pooling

Optimize for mobile.

---

PHASE 12 — LIGHTING AND EFFECTS

Create a 2D lighting system.

Support:

- ambient light
- point lights
- directional-style 2D lighting where appropriate
- light radius
- falloff
- shadows where practical
- normal maps
- sprite lighting
- blend modes

Add:

- shader materials
- custom shaders
- screen effects
- bloom-like effects where mobile performance permits
- vignette
- color adjustments

Every expensive effect must have quality/performance settings.

---

PHASE 13 — AUDIO + INPUT

Complete:

- Audio Manager
- Audio Sources
- Audio Buses
- mixer controls
- music manager
- SFX manager
- touch actions
- gestures
- virtual joystick
- virtual buttons
- controller support
- keyboard support where available

Create a visual Input Action editor.

---

PHASE 14 — UI EDITOR

Build a Unity-like 2D UI editor.

Editor panels:

Hierarchy
Inspector
Scene View
Game View
Project/Assets
Console
Animator
Animation
Tilemap
Profiler

UI editor must support:

- drag/drop
- resizing
- anchors
- snapping
- alignment
- hierarchy editing
- property editing
- preview

Make controls comfortable on touchscreens.

---

PHASE 15 — PREFABS + SERIALIZATION

Create a prefab system.

Support:

- create prefab
- edit prefab
- instantiate prefab
- nested prefabs where practical
- prefab overrides
- apply changes
- revert changes

Serialization must be:

- versioned
- validated
- recoverable
- backward-compatible where possible

Never corrupt project data after a failed save.

---

PHASE 16 — SCRIPTING SYSTEM

Create a safe and practical game scripting system.

The scripting API should expose:

- entities
- components
- transforms
- sprites
- animation
- physics
- audio
- input
- UI
- scenes
- timers
- events

Provide lifecycle methods such as:

OnCreate()
OnStart()
OnUpdate()
OnFixedUpdate()
OnDestroy()

Expose a stable engine API rather than allowing scripts to manipulate internal engine memory directly.

---

PHASE 17 — EDITOR TOOLS + DEBUGGING

Create:

- Console
- Logs
- warnings
- errors
- search
- filters
- scene debugger
- physics debugger
- renderer debugger
- asset debugger
- performance monitor

Profiler metrics:

- FPS
- frame time
- CPU time
- GPU time where available
- draw calls
- triangles
- texture memory
- total memory
- asset memory
- physics time
- audio time

---

PHASE 18 — BUILD SYSTEM

Create a real Android build pipeline.

Support:

- Debug APK
- Release APK
- Android App Bundle
- configurable package/application ID
- application name
- version name
- version code
- app icon
- splash screen where supported
- minimum SDK
- target SDK
- architectures such as ARM64
- signing configuration
- build variants

Provide:

Build Game

The user should be able to build the current project without manually editing Gradle files for normal workflows.

Show:

- build progress
- warnings
- errors
- final output
- build logs

---

PHASE 19 — PERFORMANCE + QUALITY

Optimize for:

- low-end Android phones
- mid-range phones
- high-end phones
- tablets

Implement:

- texture compression options
- atlas optimization
- sprite batching
- object pooling
- asset streaming
- memory budgets
- frame pacing
- resolution scaling
- quality presets
- battery-aware performance options

Quality presets:

Low
Medium
High
Ultra
Custom

Provide automatic recommendations based on device capabilities.

---

PHASE 20 — FINAL INTEGRATION + QA

Before declaring the engine complete:

Run:

- unit tests
- integration tests
- renderer tests
- asset tests
- serialization tests
- physics tests
- input tests
- UI tests
- build tests
- Android device tests
- low-memory tests
- malformed-asset tests
- missing-file tests
- interrupted-save tests
- interrupted-build tests

Test multiple:

- screen resolutions
- aspect ratios
- DPI values
- Android versions
- phone sizes
- tablet sizes
- orientations

The engine must fail gracefully.

Never hide errors.

Never report "success" if an operation failed.

---

EDITOR EXPERIENCE

The editor should provide a professional workflow.

Main interface:

┌────────────────────────────────────────────┐
│ File Edit View Tools Build Run Settings    │
├──────────┬─────────────────────┬───────────┤
│Hierarchy │                     │ Inspector │
│          │     Scene View      │           │
│Entities  │                     │ Properties│
│          │                     │           │
├──────────┴─────────────────────┴───────────┤
│ Assets / Project                            │
├────────────────────────────────────────────┤
│ Console / Animation / Tilemap / Profiler    │
└────────────────────────────────────────────┘

On small phones, use responsive layouts and navigation.

On tablets, support multi-panel layouts.

Do not simply shrink desktop UI.

Create mobile-specific interaction patterns.

---

TOUCH-FIRST EDITOR

The editor must support:

- tap
- double tap
- long press
- drag
- pinch zoom
- two-finger pan
- multi-selection where practical
- touch-friendly handles
- touch-friendly buttons
- contextual menus
- large enough hit targets

Provide optional:

Compact UI

and

Advanced UI

modes.

---

PROJECT MANAGEMENT

Create:

- New Project
- Open Project
- Recent Projects
- Duplicate Project
- Import Project
- Export Project
- Project Settings
- Project Backup
- Project Recovery

Provide automatic backup/recovery.

---

ASSET MANAGEMENT

Asset browser must support:

- folders
- search
- filtering
- sorting
- thumbnails
- list view
- grid view
- favorites
- recent assets
- tags
- metadata
- dependency visualization

Allow:

- rename
- move
- duplicate
- delete
- import
- reimport

Protect against accidental destructive operations.

---

FILE SYSTEM SAFETY

Never assume unrestricted Android filesystem access.

Use Android-supported storage APIs.

Support project storage using an appropriate app/project directory.

For user-selected external files, use Android document/file picker mechanisms.

If a selected URI cannot be accessed permanently, copy the asset into the project when the user chooses "Import".

Handle permission loss gracefully.

---

API DESIGN

Create a clean public engine API.

Example:

Engine::Initialize();
Engine::Update(deltaTime);
Engine::Render();
Engine::Shutdown();

Scene API:

Scene scene;
Entity player = scene.CreateEntity("Player");
player.AddComponent<Transform2D>();
player.AddComponent<SpriteRenderer>();

Input:

if (Input::IsActionPressed("Jump"))
{
    player.GetComponent<Rigidbody2D>().ApplyImpulse(...);
}

Audio:

Audio::Play("jump.wav");

Scene loading:

SceneManager::Load("Level01");

Do not expose unnecessary internal implementation details.

---

ERROR HANDLING

Every major API must return or expose meaningful failure information.

Examples:

AssetNotFound
AssetCorrupted
UnsupportedFormat
PermissionDenied
OutOfMemory
InvalidScene
InvalidComponent
ShaderCompilationFailed
TextureCreationFailed
BuildFailed
SerializationFailed

Display human-readable explanations.

Provide recovery suggestions where possible.

---

UNDO / REDO

Editor actions that modify project state should support undo/redo.

Examples:

- move entity
- delete entity
- create entity
- modify property
- paint tile
- delete tile
- modify animation
- modify UI
- modify component

Use command-based editor architecture where appropriate.

---

PERFORMANCE REQUIREMENTS

Do not optimize prematurely, but architecture must allow optimization.

Avoid:

- unnecessary per-frame allocations
- excessive virtual calls in hot loops
- unnecessary texture uploads
- unnecessary GPU state changes
- unnecessary scene traversal
- blocking disk I/O on the render thread
- blocking asset imports on the UI thread

Use profiling before making performance claims.

---

SECURITY AND ROBUSTNESS

Validate:

- project files
- asset metadata
- serialized data
- imported files
- scripts
- paths

Prevent:

- path traversal
- invalid project references
- infinite recursion
- corrupted serialization
- unbounded memory usage
- unsafe file operations

---

DOCUMENTATION

Create documentation for:

- installation
- creating projects
- importing assets
- creating scenes
- entities
- components
- physics
- animation
- tilemaps
- particles
- UI
- audio
- scripting
- building APK
- building AAB
- troubleshooting
- performance optimization
- API reference

Every public API should have documentation.

---

TESTING POLICY

For every major feature:

1. Implement.
2. Compile.
3. Run tests.
4. Test failure cases.
5. Test Android.
6. Test touch.
7. Test phone.
8. Test tablet.
9. Profile.
10. Fix regressions.
11. Document.
12. Only then mark the feature complete.

Maintain a regression test suite.

---

DEVELOPMENT WORKFLOW

Do NOT attempt to generate the entire engine in one giant untested code dump.

Implement one phase at a time.

At the end of each phase:

BUILD
↓
TEST
↓
VALIDATE
↓
FIX
↓
PROFILE
↓
DOCUMENT
↓
COMMIT
↓
NEXT PHASE

Never move to the next phase while the previous phase has unresolved critical errors.

---

DEFINITION OF DONE

A feature is DONE only when:

- code exists
- code compiles
- feature works
- error handling exists
- tests exist
- Android behavior is verified
- phone behavior is verified
- tablet behavior is verified
- documentation exists
- no known critical bug remains

Do not mark incomplete features as complete.

---

FINAL PRODUCT GOAL

The final product must be a genuine, usable:

2D Mobile Game Engine + 2D Game Editor

It should allow a developer to:

1. Create a project.
2. Import images from the phone.
3. Organize assets.
4. Create scenes.
5. Create entities.
6. Add components.
7. Create sprites.
8. Create animations.
9. Build tilemaps.
10. Add physics.
11. Add particles.
12. Add lighting/effects.
13. Add audio.
14. Create UI.
15. Write game logic.
16. Test the game.
17. Debug the game.
18. Profile the game.
19. Build an APK.
20. Build an AAB.
21. Install/run the game on Android.
22. Continue editing the project later without losing data.

The result must be a coherent engine, not a collection of unrelated tools.

Prioritize correctness, stability, maintainability, mobile performance, usability, and real functionality over superficial feature count.

When a requested feature cannot technically be implemented exactly as specified, explain the limitation and implement the closest production-quality alternative rather than creating fake functionality.
