#pragma once

#include <vengine/Common.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/scene/Registry.hpp>
#include <vengine/scene/Systems.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vengine::scene {

/// A Scene is a named container of entities + components with a tick function.
///
/// Lifecycle (matches README Engine API):
///   scene.on_create();   // user binds systems/scripts
///   scene.update(dt);    // engine ticks the scene each frame
///   scene.destroy();     // tears everything down
class Scene {
public:
    explicit Scene(std::string name = {}) : name_(std::move(name)) {}
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;

    const std::string& name() const noexcept { return name_; }

    Registry& registry() noexcept { return registry_; }
    const Registry& registry() const noexcept { return registry_; }

    /// Event bus for this scene (collisions, triggers, game events).
    EventBus& events() noexcept { return events_; }
    const EventBus& events() const noexcept { return events_; }

    Entity create_entity(std::string name = {}) {
        return registry_.create(std::move(name));
    }

    void destroy_entity(Entity e) { registry_.destroy(e); }

    // ---- systems -----------------------------------------------------------

    /// Bind a per-frame system (Update phase, default priority). Kept for the
    /// README-style lambda API; prefer attach_system for phase control.
    using SystemFn = std::function<void(Scene&, float)>;
    void add_system(SystemFn fn) {
        scheduler_.attach(std::make_shared<FunctionSystem>(UpdatePhase::Update, 0, std::move(fn)));
    }
    /// Attach a full system object with phase/priority control.
    void attach_system(std::shared_ptr<ISystem> sys) { scheduler_.attach(std::move(sys)); }

    void on_create() {
        scheduler_.on_attach(*this);
    }

    /// Run the fixed + variable update phases for one frame. The engine calls
    /// this with the frame dt; fixed-step accumulation is handled by the
    /// Engine facade so Scene stays simple.
    void update(float dt) {
        scheduler_.tick(*this, UpdatePhase::Update, dt);
        scheduler_.tick(*this, UpdatePhase::LateUpdate, dt);
    }
    void fixed_update(float dt) {
        scheduler_.tick(*this, UpdatePhase::FixedUpdate, dt);
    }
    void render() {
        scheduler_.tick(*this, UpdatePhase::Render, 0.0f);
    }

    void destroy() {
        scheduler_.on_detach(*this);
        scheduler_.clear();
        events_.clear();
    }

    std::size_t entity_count() const noexcept { return registry_.size(); }
    std::size_t system_count() const noexcept { return scheduler_.count(); }

private:
    std::string      name_;
    Registry         registry_;
    EventBus         events_;
    SystemScheduler  scheduler_;
};

/// A simple scene manager: name -> scene, with load/unload semantics. The
/// README API is SceneManager::Load("Level01").
class SceneManager {
public:
    using SceneFactory = std::function<std::unique_ptr<Scene>()>;

    void register_scene(std::string name, SceneFactory factory) {
        factories_[name] = std::move(factory);
    }

    /// Load (or reload) a named scene. Returns an error if unknown.
    Result<void> load(std::string_view name) {
        std::string key{name};
        auto it = factories_.find(key);
        if (it == factories_.end()) {
            return Result<void>::Fail(ErrorCode::FileNotFound,
                "unknown scene '" + key + "'");
        }
        if (current_) current_->destroy();
        current_ = it->second();
        current_->on_create();
        return Result<void>::Ok();
    }

    Scene* current() noexcept { return current_.get(); }
    const Scene* current() const noexcept { return current_.get(); }

    void unload() {
        if (current_) current_->destroy();
        current_.reset();
    }

private:
    std::unordered_map<std::string, SceneFactory> factories_;
    std::unique_ptr<Scene>                        current_;
};

} // namespace vengine::scene
