#pragma once

// V Engine 2.0 — Editor dashboard panels (Phase 6).
// Self-contained panel models the EditorDashboard lays out and renders.
// Each panel exposes a build() that appends rows of text/actions the
// dashboard draws through the real UI::Canvas.
#include <vengine/scene/Scene.hpp>

#include <functional>
#include <string>
#include <vector>

namespace vengine::editor {

struct PanelRow {
    std::string text;
    std::string action_id;  // non-empty → clickable button
};

class Panel {
public:
    explicit Panel(std::string title) : title_(std::move(title)) {}
    virtual ~Panel() = default;
    virtual void build(std::vector<PanelRow>& out) = 0;
    bool visible{true};
    const std::string& title() const { return title_; }
protected:
    std::string title_;
};

/// Scene hierarchy: entity tree with selection.
class HierarchyPanel : public Panel {
public:
    HierarchyPanel() : Panel("Hierarchy") {}
    void set_scene(scene::Scene* s) { scene_ = s; }
    void build(std::vector<PanelRow>& out) override {
        if (!scene_ || !visible) return;
        out.push_back({title_, ""});
        scene_->registry().each([&](scene::Entity e){
            out.push_back({"  " + scene_->registry().name(e), "select"});
        });
    }
    scene::Entity selected{};
private:
    scene::Scene* scene_{nullptr};
};

/// Inspector: property editor for the selected entity.
class InspectorPanel : public Panel {
public:
    InspectorPanel() : Panel("Inspector") {}
    void set_scene(scene::Scene* s) { scene_ = s; }
    void select(scene::Entity e) { selected = e; }
    void build(std::vector<PanelRow>& out) override {
        if (!scene_ || !visible || !selected) return;
        out.push_back({std::string("Entity: ") + scene_->registry().name(selected), ""});
    }
    scene::Entity selected{};
private:
    scene::Scene* scene_{nullptr};
};

/// Scene view: render target preview + transform gizmos.
class SceneViewPanel : public Panel {
public:
    SceneViewPanel() : Panel("Scene") {}
    void build(std::vector<PanelRow>& out) override {
        if (!visible) return;
        out.push_back({"[Scene viewport]", ""});
    }
    float zoom{1.0f};
};

/// Console: log output with severity filters.
class ConsolePanel : public Panel {
public:
    ConsolePanel() : Panel("Console") {}
    void log(const std::string& msg) { lines_.push_back(msg); }
    void build(std::vector<PanelRow>& out) override {
        if (!visible) return;
        out.push_back({title_, ""});
        for (auto& l : lines_) out.push_back({l, ""});
    }
    void clear() { lines_.clear(); }
private:
    std::vector<std::string> lines_;
};

/// Asset browser: file tree + thumbnail grid of imported assets.
class AssetBrowserPanel : public Panel {
public:
    AssetBrowserPanel() : Panel("Assets") {}
    void set_assets(const std::vector<std::string>& paths) { paths_ = paths; }
    void build(std::vector<PanelRow>& out) override {
        if (!visible) return;
        out.push_back({title_, ""});
        for (auto& p : paths_) out.push_back({p, "import"});
    }
    std::string current_dir;
private:
    std::vector<std::string> paths_;
};

/// Toolbar: play/pause/stop, build, settings buttons.
class ToolbarPanel : public Panel {
public:
    ToolbarPanel() : Panel("Toolbar") {}
    std::function<void()> on_play, on_pause, on_stop, on_build;
    void build(std::vector<PanelRow>& out) override {
        if (!visible) return;
        out.push_back({"Play", "play"});
        out.push_back({"Pause", "pause"});
        out.push_back({"Stop", "stop"});
        out.push_back({"Build", "build"});
    }
};

} // namespace vengine::editor
