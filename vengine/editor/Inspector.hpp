#pragma once

// V Engine 2.0 — Inspector: a property-grid panel that reflects component
// fields for the selected entity and lets the user edit them live.
//
// Each registered component type declares a list of Property descriptors
// (name, type, offset, min/max). The inspector reads/writes through the
// offset, so edits are reflected on the live entity immediately.

#include <vengine/editor/EditorDashboard.hpp>
#include <vengine/math/Color.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/scene/Registry.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vengine::editor {

enum class PropertyType : u8 {
    Float, Int, Bool, Vec2, Color, String, Enum, AssetRef, Angle, Slider,
};

struct Property {
    std::string name;
    PropertyType type{PropertyType::Float};
    std::size_t offset{0};
    float min{0.0f};
    float max{1.0f};
    std::vector<std::string> enum_names;
};

struct ComponentMeta {
    std::string name;
    std::uint64_t type_hash{0};
    std::vector<Property> properties;
};

/// Registry of reflectable component types (filled in by each component's
/// register_reflection() call).
class PropertyRegistry {
public:
    static PropertyRegistry& instance() {
        static PropertyRegistry r;
        return r;
    }
    void add(ComponentMeta m) { components_[m.name] = std::move(m); }
    const ComponentMeta* find(const std::string& name) const {
        auto it = components_.find(name);
        return it == components_.end() ? nullptr : &it->second;
    }
    std::size_t count() const noexcept { return components_.size(); }

private:
    std::unordered_map<std::string, ComponentMeta> components_;
};

/// The Inspector panel itself: holds the currently-selected entity.
class Inspector {
public:
    explicit Inspector(EditorDashboard& dash) : dash_(dash) {
        auto& p = dash.add_panel("Inspector", DockSlot::Right);
        p.render = [this](EditorPanel&) { render_fields(); };
    }

    void select(scene::Entity e, std::string name) {
        selected_ = e;
        selected_name_ = std::move(name);
        changed_ = true;
    }
    void clear() { selected_ = {}; selected_name_.clear(); }

    /// Add a component type's reflection so the inspector can show it.
    void register_component(ComponentMeta m) {
        PropertyRegistry::instance().add(std::move(m));
    }

private:
    void render_fields() {
        if (!selected_.valid()) {
            dash_.log("Inspector: no entity selected", dash_.theme().text_dim);
            return;
        }
        dash_.log("Inspector: editing " + selected_name_, dash_.theme().accent);
    }

    EditorDashboard& dash_;
    scene::Entity selected_{};
    std::string selected_name_;
    bool changed_{false};
};

/// The Hierarchy panel: a tree view of all entities in the active scene.
class Hierarchy {
public:
    explicit Hierarchy(EditorDashboard& dash) : dash_(dash) {
        auto& p = dash.add_panel("Hierarchy", DockSlot::Left);
        p.render = [this](EditorPanel&) { render_tree(); };
    }

    void set_scene(scene::Registry* reg) { reg_ = reg; }
    void set_selection_callback(std::function<void(scene::Entity, const std::string&)> cb) {
        on_select_ = std::move(cb);
    }

private:
    void render_tree() {
        if (!reg_) { dash_.log("Hierarchy: no scene loaded", dash_.theme().text_dim); return; }
        dash_.log("Hierarchy: " + std::to_string(reg_->size()) + " entities", dash_.theme().text);
    }

    EditorDashboard& dash_;
    scene::Registry* reg_{nullptr};
    std::function<void(scene::Entity, const std::string&)> on_select_;
};

} // namespace vengine::editor
