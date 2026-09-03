#pragma once

// V Engine 2.0 — Audio mixing bus graph (Phase 2 extension): routes voices
// through buses (master, music, sfx, ui, ambient) with per-bus volume, mute,
// solo, and a ducking sidechain. Each bus sums its children; the master bus
// is the graph root the platform audio backend pulls from.
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace vengine::audio {

struct Bus {
    std::string name;
    float volume{1.0f};
    bool muted{false};
    bool soloed{false};
    std::uint32_t parent{0};        // 0 = master/root
    std::vector<std::uint32_t> children;
    float duck_amount{0.0f};        // 0..1 applied to effective volume
    float effective_volume() const {
        if (muted) return 0.0f;
        return volume * (1.0f - duck_amount);
    }
};

class BusGraph {
public:
    BusGraph() {
        // index 0 reserved as master root
        buses_.push_back({"master", 1.0f, false, false, 0, {}, 0.0f});
    }

    std::uint32_t add_bus(const std::string& name, std::uint32_t parent) {
        std::uint32_t id = static_cast<std::uint32_t>(buses_.size());
        buses_.push_back({name, 1.0f, false, false, parent, {}, 0.0f});
        if (parent < buses_.size()) buses_[parent].children.push_back(id);
        return id;
    }

    Bus& bus(std::uint32_t id) { return buses_[id]; }
    const Bus& bus(std::uint32_t id) const { return buses_[id]; }
    std::size_t count() const { return buses_.size(); }

    /// Resolve effective volume considering solo: if any bus is soloed, only
    /// soloed buses (and their ancestors) are audible.
    float resolved_volume(std::uint32_t id) const {
        if (any_soloed()) {
            // audible only if this bus or a descendant is soloed (and ancestors)
            if (!soloed_or_ancestor_of_soloed(id)) return 0.0f;
        }
        float v = buses_[id].effective_volume();
        // apply ancestor gains
        std::uint32_t p = buses_[id].parent;
        while (p != id && p < buses_.size()) {
            v *= buses_[p].effective_volume();
            p = buses_[p].parent;
            if (p == 0) { v *= buses_[0].effective_volume(); break; }
        }
        return v;
    }

    void set_duck(std::uint32_t id, float amount) {
        if (id < buses_.size()) buses_[id].duck_amount = std::clamp(amount, 0.0f, 1.0f);
    }

private:
    bool any_soloed() const {
        for (auto& b : buses_) if (b.soloed) return true;
        return false;
    }
    bool soloed_or_ancestor_of_soloed(std::uint32_t id) const {
        // audible if id is soloed, or a descendant of id is soloed
        std::vector<std::uint32_t> stack{id};
        while (!stack.empty()) {
            auto cur = stack.back(); stack.pop_back();
            if (buses_[cur].soloed) return true;
            for (auto c : buses_[cur].children) stack.push_back(c);
        }
        return false;
    }
    std::vector<Bus> buses_;
};

} // namespace vengine::audio
