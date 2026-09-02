#pragma once

// V Engine 2.0 — Editor dashboard: the runtime editor surface with panels
// for scene hierarchy, inspector, asset browser, console, profiler, and a
// toolbar. Built on the retained Widget tree (UI.hpp).
//
// This is the in-game "editor mode" overlay: when toggled on, the engine
// renders the dashboard over the running game. It is dockable, themeable,
// and scriptable.

#include <vengine/core/Types.hpp>
#include <vengine/math/Color.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/ui/UI.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace vengine::editor {

enum class DockSlot : u8 { Left, Right, Top, Bottom, Center, Floating };

struct EditorTheme {
    math::Color bg{0.12f, 0.12f, 0.14f, 1.0f};
    math::Color panel{0.16f, 0.16f, 0.18f, 1.0f};
    math::Color panel_alt{0.20f, 0.20f, 0.22f, 1.0f};
    math::Color border{0.30f, 0.30f, 0.34f, 1.0f};
    math::Color text{0.88f, 0.88f, 0.92f, 1.0f};
    math::Color text_dim{0.55f, 0.55f, 0.60f, 1.0f};
    math::Color accent{0.28f, 0.58f, 0.92f, 1.0f};
    math::Color accent_hover{0.38f, 0.68f, 1.00f, 1.0f};
    math::Color warning{0.95f, 0.76f, 0.20f, 1.0f};
    math::Color error{0.92f, 0.32f, 0.32f, 1.0f};
    math::Color success{0.36f, 0.78f, 0.42f, 1.0f};
    float font_size{14.0f};
    float row_height{22.0f};
    float panel_min_w{180.0f};
    float tab_height{26.0f};
};

struct EditorPanel {
    std::string name;
    DockSlot dock{DockSlot::Right};
    bool open{true};
    bool focused{false};
    std::shared_ptr<ui::Widget> root;
    std::function<void(EditorPanel&)> render;
    float width{280.0f};
    float height{400.0f};
};

class EditorDashboard {
public:
    explicit EditorDashboard(EditorTheme theme = {}) : theme_(theme) {
        root_ = std::make_shared<ui::Widget>();
        root_->id = "editor_dashboard";
        root_->type = ui::WidgetType::Panel;
        root_->color = theme_.bg;
    }

    EditorPanel& add_panel(std::string name, DockSlot dock = DockSlot::Right) {
        auto p = std::make_shared<EditorPanel>();
        p->name = std::move(name);
        p->dock = dock;
        p->root = std::make_shared<ui::Widget>();
        p->root->id = p->name + "_root";
        p->root->type = ui::WidgetType::Panel;
        p->root->color = theme_.panel;
        root_->children.push_back(p->root);
        panels_.push_back(std::move(p));
        return *panels_.back();
    }

    void toggle() { visible_ = !visible_; }
    bool visible() const noexcept { return visible_; }
    void show() noexcept { visible_ = true; }
    void hide() noexcept { visible_ = false; }

    std::shared_ptr<ui::Widget> root() noexcept { return root_; }
    EditorTheme& theme() noexcept { return theme_; }
    std::vector<std::shared_ptr<EditorPanel>>& panels() noexcept { return panels_; }

    /// Recompute panel layout: docked panels snap to screen edges.
    void layout(math::Vec2f screen) {
        root_->rect = {0, 0, screen.x, screen.y};
        root_->absolute = root_->rect;
        float left_w = 0, right_w = 0, top_h = 0, bottom_h = 0;
        for (auto& p : panels_) {
            if (!p->open) continue;
            switch (p->dock) {
                case DockSlot::Left:   p->root->rect = {left_w, top_h, p->width, screen.y - top_h - bottom_h}; left_w += p->width; break;
                case DockSlot::Right:  right_w += p->width; p->root->rect = {screen.x - right_w, top_h, p->width, screen.y - top_h - bottom_h}; break;
                case DockSlot::Top:    p->root->rect = {0, top_h, screen.x, p->height}; top_h += p->height; break;
                case DockSlot::Bottom: bottom_h += p->height; p->root->rect = {0, screen.y - bottom_h, screen.x, p->height}; break;
                case DockSlot::Center:  p->root->rect = {left_w, top_h, screen.x - left_w - right_w, screen.y - top_h - bottom_h}; break;
                case DockSlot::Floating: break;
            }
            p->root->absolute = p->root->rect;
            if (p->render) p->render(*p);
        }
    }

    EditorPanel* find(const std::string& name) {
        for (auto& p : panels_) if (p->name == name) return p.get();
        return nullptr;
    }

    struct LogEntry { std::string text; math::Color color; float time; };
    void log(std::string text, math::Color color = {}) {
        if (color.a == 0) color = theme_.text;
        console_.push_back({std::move(text), color, 0.0f});
        if (console_.size() > 500) console_.erase(console_.begin());
    }
    std::vector<LogEntry>& console() noexcept { return console_; }

private:
    EditorTheme theme_;
    std::shared_ptr<ui::Widget> root_;
    std::vector<std::shared_ptr<EditorPanel>> panels_;
    std::vector<LogEntry> console_;
    bool visible_{false};
};

/// Build the standard dashboard: hierarchy, inspector, asset browser,
/// console, profiler, toolbar + play controls.
inline std::unique_ptr<EditorDashboard> make_default_dashboard() {
    auto dash = std::make_unique<EditorDashboard>();
    dash->add_panel("Hierarchy", DockSlot::Left);
    dash->add_panel("Inspector", DockSlot::Right);
    dash->add_panel("Asset Browser", DockSlot::Bottom);
    dash->add_panel("Console", DockSlot::Bottom);
    dash->add_panel("Profiler", DockSlot::Right);
    dash->add_panel("Toolbar", DockSlot::Top);
    dash->log("V Engine Editor ready.", dash->theme().accent);
    return dash;
}

} // namespace vengine::editor
