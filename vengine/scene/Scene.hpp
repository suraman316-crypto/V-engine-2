#pragma once

#include <vengine/Common.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/scene/Registry.hpp>

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

    Entity create_entity(std::string name = {}) {
        return registry_.create(std::move(name));
    }

    void destroy_entity(Entity e) { registry_.destroy(e); }

    /// Bind a per-frame system. Systems run in registration order; this is
    /// deterministic and predictable for game devs.
    using SystemFn = std::function<void(Scene&, float)>;
    void add_system(SystemFn fn) { systems_.push_back(std::move(fn)); }

    void on_create() {}
    void update(float dt) {
        for (auto& sys : systems_) sys(*this, dt);
    }
    void destroy() {
        systems_.clear();
    }

    std::size_t entity_count() const noexcept { return registry_.size(); }

private:
    std::string           name_;
    Registry              registry_;
    std::vector<SystemFn> systems_;
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
