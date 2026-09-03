#pragma once

// Native C++ scripting: a ScriptBehaviour base + NativeScriptRuntime.
//
// The README specifies a scripting layer; the long-term plan is a Lua/QuickJS
// VM behind IScriptRuntime, but gameplay is best written in C++ for
// performance-critical systems. This module provides the C++ path: derive
// from ScriptBehaviour, override on_create/on_update/on_destroy, and register
// the type with the runtime. The runtime instantiates behaviours per-entity
// and ticks them each frame, exactly like a Unity MonoBehaviour.

#include <vengine/Common.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/scene/Scene.hpp>
#include <vengine/scripting/Scripting.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace vengine::scripting {

class ScriptBehaviour {
public:
    virtual ~ScriptBehaviour() = default;
    scene::Entity entity{};
    scene::Scene* scene{nullptr};

    virtual void on_create() {}
    virtual void on_update(float /*dt*/) {}
    virtual void on_destroy() {}
};

/// Factory: returns a fresh behaviour instance for a given entity.
using BehaviourFactory = std::function<std::unique_ptr<ScriptBehaviour>()>;

/// Registry of script types by name. Gameplay registers types at startup.
class BehaviourRegistry {
public:
    static BehaviourRegistry& instance() {
        static BehaviourRegistry r;
        return r;
    }
    void register_type(std::string name, BehaviourFactory factory) {
        factories_[std::move(name)] = std::move(factory);
    }
    BehaviourFactory find(std::string_view name) const {
        auto it = factories_.find(std::string{name});
        return it == factories_.end() ? BehaviourFactory{} : it->second;
    }
private:
    std::unordered_map<std::string, BehaviourFactory> factories_;
};

/// Convenience macro to register a ScriptBehaviour subclass at static init.
#define VENGINE_REGISTER_SCRIPT(Type) \
    namespace { struct Type##_reg { Type##_reg() { \
        vengine::scripting::BehaviourRegistry::instance().register_type( \
            #Type, []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>( \
                std::make_unique<Type>()); }); } }; \
        static Type##_reg Type##_reg_instance; }

/// A behaviour instance bound to an entity, plus its started flag.
struct BehaviourInstance {
    std::unique_ptr<ScriptBehaviour> behaviour;
    bool started{false};
};

/// Runtime: tracks behaviours across the active scene, ticks them, and
/// implements IScriptRuntime so it can be installed into the Engine.
class NativeScriptRuntime : public IScriptRuntime {
public:
    Result<void> initialize() override { return Result<void>::Ok(); }
    void         shutdown() override { instances_.clear(); }

    /// Attach a registered behaviour type to an entity. Returns false if the
    /// type isn't registered.
    bool attach(scene::Entity e, std::string_view type_name) {
        auto factory = BehaviourRegistry::instance().find(type_name);
        if (!factory) return false;
        BehaviourInstance inst;
        inst.behaviour = factory();
        inst.behaviour->entity = e;
        instances_[e.index()].push_back(std::move(inst));
        return true;
    }

    /// Attach an already-constructed behaviour (for tests / direct C++ use).
    void attach(scene::Entity e, std::unique_ptr<ScriptBehaviour> b) {
        b->entity = e;
        instances_[e.index()].push_back(BehaviourInstance{std::move(b), false});
    }

    void tick(scene::Scene& scene, float dt) override {
        for (auto& [idx, list] : instances_) {
            for (auto& inst : list) {
                if (!inst.behaviour) continue;
                if (!inst.started) {
                    inst.behaviour->scene = &scene;
                    inst.behaviour->on_create();
                    inst.started = true;
                }
                inst.behaviour->on_update(dt);
            }
        }
    }

    Result<void> add_system(std::string_view, std::string_view) override {
        return Result<void>::Ok(); // native runtime ignores script source
    }

    std::size_t instance_count() const noexcept {
        std::size_t n = 0;
        for (const auto& [idx, list] : instances_) n += list.size();
        return n;
    }

private:
    std::unordered_map<std::uint32_t, std::vector<BehaviourInstance>> instances_;
};

} // namespace vengine::scripting
