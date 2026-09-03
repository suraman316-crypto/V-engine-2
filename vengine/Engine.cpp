#include <vengine/Engine.hpp>
#include <vengine/core/Assert.hpp>
#include <vengine/core/Logging.hpp>
#include <vengine/profiler/Profiler.hpp>

namespace vengine {

Engine::Engine() = default;
Engine::~Engine() { if (running_) shutdown(); }

Result<void> Engine::initialize() {
    VENGINE_PROFILE("Engine::Initialize");

    if (!platform_.window) {
        VENGINE_LOG_WARN("Engine", "initialized without a window (headless/editor mode)");
    }

    if (renderer_) {
        void* native = platform_.window ? platform_.window->native_handle() : nullptr;
        auto res = renderer_->initialize(native);
        if (!res.ok()) return res.error();
    }
    if (physics_) {
        auto res = physics_->initialize({0.0f, -9.81f});
        if (!res.ok()) return res.error();
    }
    if (audio_) {
        auto res = audio_->initialize();
        if (!res.ok()) return res.error();
    }
    if (scripting_) {
        auto res = scripting_->initialize();
        if (!res.ok()) return res.error();
    }

    running_ = true;
    VENGINE_LOG_INFO("Engine", "V Engine initialized (backend=%s)",
                     renderer_ ? (renderer_->backend() == renderer::Backend::OpenGLES32
                                  ? "OpenGLES32" : "Vulkan")
                              : "none");
    return Result<void>::Ok();
}

void Engine::update(float dt) {
    VENGINE_PROFILE("Engine::Update");
    if (!running_) return;

    // Camera follow/shake ahead of gameplay so systems read the settled view.
    camera_.update(dt, rng_);
    // Tweens advance on the Update phase so gameplay reads interpolated values.
    tweens_.update(dt);

    if (auto* s = scene_manager_.current()) {
        s->update(dt);
        // Drive native script behaviours (the C++ scripting path).
        if (native_scripts_) native_scripts_->tick(*s, dt);
    }
    if (physics_) physics_->step(dt, 8, 3);
    if (scripting_) {
        if (auto* s = scene_manager_.current()) scripting_->tick(*s, dt);
    }
    input_.end_frame();
}

void Engine::render() {
    VENGINE_PROFILE("Engine::Render");
    if (!running_ || !renderer_) return;
    if (!renderer_->is_valid()) return;

    // Prefer the Camera2D state (follow/shake) when set; otherwise read the
    // active Camera component from the scene for backward compatibility.
    renderer::Camera cam{};
    bool cam2d_active = camera_.follow || camera_.shake_magnitude > 0.0f;
    if (cam2d_active) {
        cam.position = camera_.shaken_position(rng_);
        cam.zoom = camera_.zoom;
        cam.rotation = camera_.rotation;
        cam.viewport = camera_.viewport;
        cam.clear_color = camera_.clear_color;
    } else if (auto* s = scene_manager_.current()) {
        s->registry().view<components::Camera, components::TransformComponent>(
            [&](scene::Entity, const components::Camera& c,
                const components::TransformComponent& tc) {
                if (c.active) {
                    cam.position = tc.value.position;
                    cam.zoom = c.zoom;
                    cam.viewport = c.viewport_size;
                    cam.clear_color = c.clear_color;
                }
            });
    }

    renderer_->begin_frame(cam);

    // Build a sorted, batched command buffer from the scene's sprites, cull
    // off-screen, then submit baked batches to the backend.
    cmd_buf_.clear();
    if (auto* s = scene_manager_.current()) {
        std::vector<renderer::SpriteDraw> draws;
        draws.reserve(64);
        s->registry().view<components::SpriteRenderer, components::TransformComponent>(
            [&](scene::Entity, const components::SpriteRenderer& spr,
                const components::TransformComponent& tc) {
                renderer::SpriteCommand sc;
                sc.transform = tc.value;
                sc.uv = spr.uv;
                sc.color = spr.color;
                sc.layer = spr.layer;
                sc.order_in_layer = spr.order_in_layer;
                sc.flip_x = spr.flip_x;
                sc.flip_y = spr.flip_y;
                cmd_buf_.submit(sc);

                renderer::SpriteDraw d;
                d.transform = tc.value;
                d.uv = spr.uv;
                d.color = spr.color;
                d.layer = spr.layer;
                d.order_in_layer = spr.order_in_layer;
                d.flip_x = spr.flip_x;
                d.flip_y = spr.flip_y;
                draws.push_back(d);
            });
        // Cull against the camera view AABB, then forward survivors.
        if (cam2d_active) cmd_buf_.cull(camera_.view_aabb(rng_));
        if (!draws.empty()) renderer_->submit_sprites(draws.data(), draws.size());
    }

    // Debug overlay (physics shapes, gizmos) -> command buffer shapes.
    debug_draw_.begin_frame();
    if (debug_draw_.enabled() && physics_) {
        // (visualization hook: a physics-debug system would draw colliders here)
    }
    debug_draw_.submit(cmd_buf_);

    renderer_->end_frame();
}

void Engine::shutdown() {
    VENGINE_PROFILE("Engine::Shutdown");
    VENGINE_LOG_INFO("Engine", "shutting down");
    if (scripting_) scripting_->shutdown();
    if (native_scripts_) native_scripts_->shutdown();
    if (audio_)     audio_->shutdown();
    if (physics_)   physics_->shutdown();
    if (renderer_)  renderer_->shutdown();
    tweens_.clear();
    scene_manager_.unload();
    running_ = false;
}

void Engine::install_renderer(std::unique_ptr<renderer::IRenderer> r)    { renderer_  = std::move(r); }
void Engine::install_physics(std::unique_ptr<physics::IPhysicsWorld> p)  { physics_   = std::move(p); }
void Engine::install_audio(std::unique_ptr<audio::IAudioEngine> a)       { audio_     = std::move(a); }
void Engine::install_scripting(std::unique_ptr<scripting::IScriptRuntime> s) { scripting_ = std::move(s); }

void Engine::install_native_scripting() {
    native_scripts_ = std::make_unique<scripting::NativeScriptRuntime>();
    // native_scripts_ is the sole owner; scripting() returns it when no
    // external IScriptRuntime was installed.
}

} // namespace vengine
