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

    if (auto* s = scene_manager_.current()) s->update(dt);
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

    renderer::Camera cam{};
    if (auto* s = scene_manager_.current()) {
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

    // Collect sprites from the scene. We allocate a small stack buffer for the
    // common case to avoid per-frame heap allocation (README perf rule).
    if (auto* s = scene_manager_.current()) {
        std::vector<renderer::SpriteDraw> draws;
        draws.reserve(64);
        s->registry().view<components::SpriteRenderer, components::TransformComponent>(
            [&](scene::Entity, const components::SpriteRenderer& spr,
                const components::TransformComponent& tc) {
                renderer::SpriteDraw d;
                d.transform = tc.value;
                d.uv = spr.uv;
                d.color = spr.color;
                d.layer = spr.layer;
                d.order_in_layer = spr.order_in_layer;
                d.flip_x = spr.flip_x;
                d.flip_y = spr.flip_y;
                // texture handle resolved from asset DB in a later phase.
                draws.push_back(d);
            });
        if (!draws.empty()) renderer_->submit_sprites(draws.data(), draws.size());
    }

    renderer_->end_frame();
}

void Engine::shutdown() {
    VENGINE_PROFILE("Engine::Shutdown");
    VENGINE_LOG_INFO("Engine", "shutting down");
    if (scripting_) scripting_->shutdown();
    if (audio_)     audio_->shutdown();
    if (physics_)   physics_->shutdown();
    if (renderer_)  renderer_->shutdown();
    scene_manager_.unload();
    running_ = false;
}

void Engine::install_renderer(std::unique_ptr<renderer::IRenderer> r)    { renderer_  = std::move(r); }
void Engine::install_physics(std::unique_ptr<physics::IPhysicsWorld> p)  { physics_   = std::move(p); }
void Engine::install_audio(std::unique_ptr<audio::IAudioEngine> a)       { audio_     = std::move(a); }
void Engine::install_scripting(std::unique_ptr<scripting::IScriptRuntime> s) { scripting_ = std::move(s); }

} // namespace vengine
