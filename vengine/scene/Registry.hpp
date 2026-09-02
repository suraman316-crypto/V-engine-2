#pragma once

#include <vengine/core/Assert.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace vengine::scene {

/// Strongly-typed entity identifier. 64-bit: low 32 = index, high 32 = version
/// (generation counter). The version lets us detect stale handles after an
/// entity is destroyed and reused, preventing dangling references.
struct Entity {
    std::uint64_t raw{0};

    constexpr Entity() = default;
    constexpr explicit Entity(std::uint64_t v) noexcept : raw(v) {}

    constexpr bool valid() const noexcept { return raw != 0; }
    constexpr explicit operator bool() const noexcept { return valid(); }
    constexpr bool operator==(const Entity&) const noexcept = default;

    constexpr std::uint32_t index()   const noexcept { return static_cast<std::uint32_t>(raw & 0xFFFFFFFFu); }
    constexpr std::uint32_t version() const noexcept { return static_cast<std::uint32_t>(raw >> 32); }

    static constexpr Entity make(std::uint32_t index, std::uint32_t version) noexcept {
        return Entity{(static_cast<std::uint64_t>(version) << 32) | static_cast<std::uint64_t>(index)};
    }
};

using ComponentTypeId = std::uint32_t;

namespace detail {
inline ComponentTypeId next_component_id() noexcept {
    static ComponentTypeId id{0};
    return id++;
}

template <typename T>
ComponentTypeId component_id() noexcept {
    static const ComponentTypeId id = next_component_id();
    return id;
}
} // namespace detail

/// The Entity-Component registry. Each component type lives in its own sparse
/// storage (index->component). NOT thread-safe; the Scene serializes access.
class Registry {
public:
    Registry() = default;
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    Entity create(std::string name = {}) {
        std::uint32_t index{};
        std::uint32_t version{};
        if (free_slots_.empty()) {
            index = static_cast<std::uint32_t>(meta_.size());
            meta_.emplace_back();           // version starts at 0
            version = 1;                    // first generation is 1, never 0
            meta_[index].version = version;
        } else {
            index = free_slots_.back();
            free_slots_.pop_back();
            version = meta_[index].version; // already bumped by destroy()
        }
        Entity e = Entity::make(index, version);
        meta_[index].alive = true;
        meta_[index].name  = std::move(name);
        return e;
    }

    void destroy(Entity e) {
        if (!alive(e)) return;
        auto idx = e.index();
        for (auto& [type_id, storage] : storages_) {
            (void)type_id;
            storage->erase(idx);
        }
        meta_[idx].alive = false;
        meta_[idx].version += 1; // bump generation -> stale handles detected
        meta_[idx].name.clear();
        free_slots_.push_back(idx);
    }

    bool alive(Entity e) const noexcept {
        if (!e.valid()) return false;
        if (e.index() >= meta_.size()) return false;
        const auto& m = meta_[e.index()];
        return m.alive && m.version == e.version();
    }

    const std::string& name(Entity e) const {
        static const std::string empty;
        if (!alive(e)) return empty;
        return meta_[e.index()].name;
    }

    std::size_t size() const noexcept {
        std::size_t n = 0;
        for (const auto& m : meta_) if (m.alive) ++n;
        return n;
    }

    template <typename T, typename... Args>
    T& add(Entity e, Args&&... args) {
        VENGINE_ASSERT(alive(e), "add component to dead entity");
        auto& storage = typed_storage<T>();
        storage.data[e.index()] = T(std::forward<Args>(args)...);
        return storage.data[e.index()];
    }

    template <typename T>
    bool has(Entity e) const {
        if (!alive(e)) return false;
        const auto* storage = typed_storage_ro<T>();
        return storage && storage->data.find(e.index()) != storage->data.end();
    }

    template <typename T>
    T* get(Entity e) {
        if (!alive(e)) return nullptr;
        auto* storage = typed_storage_ro<T>();
        if (!storage) return nullptr;
        auto it = storage->data.find(e.index());
        return it == storage->data.end() ? nullptr : &it->second;
    }

    template <typename T>
    const T* get(Entity e) const {
        if (!alive(e)) return nullptr;
        const auto* storage = typed_storage_ro<T>();
        if (!storage) return nullptr;
        auto it = storage->data.find(e.index());
        return it == storage->data.end() ? nullptr : &it->second;
    }

    template <typename T>
    void remove(Entity e) {
        if (!alive(e)) return;
        if (auto* storage = typed_storage_ro<T>()) storage->data.erase(e.index());
    }

    /// Iterate entities that have ALL of T... components.
    template <typename... T, typename F>
    void view(F&& fn) {
        view_impl<T...>(std::forward<F>(fn));
    }

    /// Const overload. The callback receives const component references.
    template <typename... T, typename F>
    void view(F&& fn) const {
        view_impl_const<T...>(std::forward<F>(fn));
    }

private:
    struct EntityMeta {
        bool          alive{false};
        std::uint32_t version{0};
        std::string   name;
    };

    struct IStorage {
        virtual ~IStorage() = default;
        virtual void erase(std::uint32_t index) = 0;
    };

    template <typename T>
    struct Storage : IStorage {
        std::unordered_map<std::uint32_t, T> data;
        void erase(std::uint32_t index) override { data.erase(index); }
    };

    template <typename T>
    Storage<T>& typed_storage() {
        const auto id = detail::component_id<T>();
        auto& slot = storages_[id];
        if (!slot) slot = std::make_unique<Storage<T>>();
        return static_cast<Storage<T>&>(*slot);
    }

    template <typename T>
    Storage<T>* typed_storage_ro() const {
        const auto id = detail::component_id<T>();
        auto it = storages_.find(id);
        if (it == storages_.end() || !it->second) return nullptr;
        return static_cast<Storage<T>*>(it->second.get());
    }

    template <typename First, typename... Rest, typename F>
    void view_impl(F&& fn) {
        auto* first = typed_storage_ro<First>();
        if (!first) return;
        for (auto& [idx, comp] : first->data) {
            if (idx >= meta_.size() || !meta_[idx].alive) continue;
            Entity e = Entity::make(idx, meta_[idx].version);
            if (!alive(e)) continue;
            if constexpr (sizeof...(Rest) == 0) {
                fn(e, comp);
            } else {
                if ((has<Rest>(e) && ...)) {
                    fn(e, comp, *get<Rest>(e)...);
                }
            }
        }
    }

    template <typename First, typename... Rest, typename F>
    void view_impl_const(F&& fn) const {
        const auto* first = typed_storage_ro<First>();
        if (!first) return;
        for (const auto& [idx, comp] : first->data) {
            if (idx >= meta_.size() || !meta_[idx].alive) continue;
            Entity e = Entity::make(idx, meta_[idx].version);
            if (!alive(e)) continue;
            if constexpr (sizeof...(Rest) == 0) {
                fn(e, comp);
            } else {
                if ((has<Rest>(e) && ...)) {
                    fn(e, comp, *get<Rest>(e)...);
                }
            }
        }
    }

    std::vector<EntityMeta>                                         meta_;
    std::vector<std::uint32_t>                                     free_slots_;
    std::unordered_map<ComponentTypeId, std::unique_ptr<IStorage>> storages_;
};

} // namespace vengine::scene
