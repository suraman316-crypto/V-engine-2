#pragma once

// Systems + event bus.
//
// A System is a per-scene update unit that iterates a component query and
// mutates state. Systems run in phases (FixedUpdate, Update, LateUpdate) so
// physics, gameplay, and camera-follow can be ordered deterministically.
//
// The EventBus decouples producers (e.g. the physics solver raising a
// Collision event) from consumers (scripts/components that registered a
// callback). Events are dispatched synchronously; for fire-and-forget game
// events this is simpler and avoids frame-delayed side effects in the
// fixed-step physics step.

#include <vengine/Common.hpp>
#include <vengine/scene/Registry.hpp>

#include <any>
#include <algorithm>
#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace vengine::scene {

class Scene;

/// Update phase. Systems are sorted within a phase by priority (lower runs
/// first). Phases always run in this fixed order so the engine pipeline is
/// predictable.
enum class UpdatePhase : u8 {
    FixedUpdate,   // physics step (may run 0..N times per frame)
    Update,        // gameplay logic
    LateUpdate,    // camera follow, post-processing
    Render,        // gather render commands
    Count
};

/// Base interface for a system. Prefer the templated System<T> below for typed
/// game systems; this interface lets the scheduler hold them uniformly.
class ISystem {
public:
    virtual ~ISystem() = default;
    virtual UpdatePhase phase() const noexcept { return UpdatePhase::Update; }
    virtual int priority() const noexcept { return 0; }
    virtual void on_attach(Scene&) {}
    virtual void on_detach(Scene&) {}
    /// Called each tick of this system's phase.
    virtual void update(Scene& scene, float dt) = 0;
};

/// A typed system: subclass and override update(). The default phase is
/// Update; override phase()/priority() to reorder.
class System : public ISystem {};

/// A function-based system for quick scripting without subclassing. Mirrors
/// Scene::add_system but participates in the phase/priority ordering.
class FunctionSystem : public ISystem {
public:
    using Fn = std::function<void(Scene&, float)>;
    FunctionSystem(UpdatePhase ph, int prio, Fn fn)
        : phase_(ph), priority_(prio), fn_(std::move(fn)) {}
    UpdatePhase phase() const noexcept override { return phase_; }
    int priority() const noexcept override { return priority_; }
    void update(Scene& scene, float dt) override { fn_(scene, dt); }
private:
    UpdatePhase phase_;
    int priority_;
    Fn fn_;
};

/// A subscriber handle returned by EventBus::subscribe. Destroying it
/// unsubscribes; copyable but the subscription lives only as long as the
/// last copy.
class Subscription {
public:
    using Unsub = std::function<void()>;
    Subscription() = default;
    explicit Subscription(Unsub u) : unsub_(std::move(u)) {}
    Subscription(const Subscription&) = default;
    Subscription(Subscription&&) noexcept = default;
    Subscription& operator=(const Subscription&) = default;
    Subscription& operator=(Subscription&&) noexcept = default;
    ~Subscription() { reset(); }
    void reset() { if (unsub_) { unsub_(); unsub_ = {}; } }
    void release() { unsub_ = {}; }
private:
    Unsub unsub_;
};

/// Type-erased pub/sub bus. Events are dispatched by C++ type. There is no
/// queuing: dispatch calls every subscriber immediately. This matches the
/// synchronous, deterministic model the README mandates for the fixed step.
class EventBus {
public:
    template <typename E>
    [[nodiscard]] Subscription subscribe(std::function<void(const E&)> cb) {
        auto& slot = slots_[std::type_index(typeid(E))];
        std::size_t index = slot.size(); // token before push
        slot.push_back([cb = std::move(cb)](const std::any& e) {
            if (auto p = std::any_cast<E>(&e)) cb(*p);
        });
        std::type_index ti(typeid(E));
        std::size_t my_index = index;
        // Capture by value so unsubscribe is stable across later pushes.
        return Subscription([this, ti, my_index]() {
            auto it = slots_.find(ti);
            if (it != slots_.end() && my_index < it->second.size()) {
                it->second[my_index] = nullptr; // tombstone
            }
        });
    }

    template <typename E>
    void publish(const E& event) {
        auto it = slots_.find(std::type_index(typeid(E)));
        if (it == slots_.end()) return;
        // Copy the vector in case a subscriber subscribes/unsubscribes mid-dispatch.
        auto copy = it->second;
        std::any wrapped = event;
        for (auto& fn : copy) {
            if (fn) fn(wrapped);
        }
    }

    void clear() noexcept { slots_.clear(); }

private:
    using AnyFn = std::function<void(const std::any&)>;
    std::unordered_map<std::type_index, std::vector<AnyFn>> slots_;
};

/// Owns the ordered list of systems attached to a scene and ticks them by
/// phase. Systems are sorted lazily after a mutation.
class SystemScheduler {
public:
    void attach(std::shared_ptr<ISystem> sys) {
        systems_.push_back(std::move(sys));
        dirty_ = true;
    }
    void detach(const std::shared_ptr<ISystem>& sys) {
        std::erase(systems_, sys);
    }
    void clear() noexcept { systems_.clear(); }

    void on_attach(Scene& scene) {
        for (auto& s : systems_) s->on_attach(scene);
    }
    void on_detach(Scene& scene) {
        for (auto& s : systems_) s->on_detach(scene);
    }

    /// Run every system whose phase matches `phase`, in priority order.
    void tick(Scene& scene, UpdatePhase phase, float dt) {
        if (dirty_) {
            sort();
            dirty_ = false;
        }
        for (auto& s : systems_) {
            if (s->phase() == phase) s->update(scene, dt);
        }
    }

    std::size_t count() const noexcept { return systems_.size(); }

private:
    void sort() {
        std::stable_sort(systems_.begin(), systems_.end(),
            [](const std::shared_ptr<ISystem>& a, const std::shared_ptr<ISystem>& b) {
                if (a->phase() != b->phase()) return false; // phase not ordered here
                return a->priority() < b->priority();
            });
    }
    std::vector<std::shared_ptr<ISystem>> systems_;
    bool dirty_ = false;
};

} // namespace vengine::scene
