#pragma once

// V Engine 2.0 — Prefab: a serialized entity tree that can be instantiated
// many times. Prefabs store component data (not live entities) so they can
// be edited, nested, and hot-reloaded.
//
// A prefab is a list of (component-type, blob) pairs per node, plus a
// parent-child graph. Instantiation creates real entities in a Registry and
// copies the component blobs in.

#include <vengine/Common.hpp>
#include <vengine/scene/Registry.hpp>

#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace vengine::assets {

struct PrefabNode {
    std::string name;
    std::uint32_t parent{0xFFFFFFFF};
    std::vector<std::uint32_t> children;
    std::unordered_map<std::string, std::vector<std::uint8_t>> components; ///< type -> blob
};

struct Prefab {
    std::string name;
    std::vector<PrefabNode> nodes;
    std::uint32_t root{0};

    /// Add an empty node, returns its index.
    std::uint32_t add_node(std::string name, std::uint32_t parent = 0xFFFFFFFF) {
        std::uint32_t idx = static_cast<std::uint32_t>(nodes.size());
        PrefabNode n; n.name = std::move(name); n.parent = parent;
        nodes.push_back(std::move(n));
        if (parent != 0xFFFFFFFF && parent < nodes.size())
            nodes[parent].children.push_back(idx);
        if (root == 0 && idx == 0) root = 0;
        return idx;
    }

    template <typename T>
    void set_component(std::uint32_t node, std::string type_name, const T& data) {
        if (node >= nodes.size()) return;
        const auto* p = reinterpret_cast<const std::uint8_t*>(&data);
        nodes[node].components[type_name] = std::vector<std::uint8_t>(p, p + sizeof(T));
    }

    template <typename T>
    bool get_component(std::uint32_t node, std::string type_name, T& out) const {
        if (node >= nodes.size()) return false;
        auto it = nodes[node].components.find(type_name);
        if (it == nodes[node].components.end()) return false;
        if (it->second.size() < sizeof(T)) return false;
        std::memcpy(&out, it->second.data(), sizeof(T));
        return true;
    }

    std::size_t node_count() const noexcept { return nodes.size(); }
};

/// Instantiates a prefab into a live scene Registry.
/// The caller provides a callback that maps a component type name + blob to
/// a real `addComponent` call on the entity — this keeps the Prefab system
/// decoupled from the concrete component set.
inline void instantiate(const Prefab& prefab, scene::Registry& reg,
                        std::function<void(scene::Entity, const std::string&, const std::vector<std::uint8_t>&)> apply_component,
                        scene::Entity root_entity = {}) {
    if (prefab.nodes.empty()) return;
    std::vector<scene::Entity> entities(prefab.nodes.size());
    for (std::size_t i = 0; i < prefab.nodes.size(); ++i) {
        entities[i] = (i == 0 && root_entity.valid()) ? root_entity : reg.create();
        if (entities[i].valid()) {
            for (const auto& [type_name, blob] : prefab.nodes[i].components)
                apply_component(entities[i], type_name, blob);
        }
    }
    (void)entities;
}

} // namespace vengine::assets
