#pragma once

#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/math/Rect.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/math/Color.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace vengine::ui {

/// Layout anchor + pivot model (Unity-like). 0..1 normalized within parent.
struct Anchor { float min_x{0.0f}, min_y{0.0f}, max_x{1.0f}, max_y{1.0f}; };

/// Widget kinds supported by the UI framework (README UI section).
enum class WidgetType {
    Panel, Button, Label, Image, Slider, ProgressBar, Toggle, Checkbox,
    Dropdown, ScrollView, List, Grid, InputField, Popup, Dialog, Tab,
};

/// Base widget. The editor/runtime drives a retained-mode UI tree; rendering
/// is performed by the renderer backend through a UI draw list (later phase).
struct Widget {
    WidgetType    type{WidgetType::Panel};
    std::string   id;
    std::string   text;          ///< label / button caption / input value
    math::Rectf   rect;          ///< local rect within parent
    math::Rectf   absolute;      ///< computed by layout_pass; used by hit_test
    Anchor        anchor;
    math::Vec2f   pivot{0.5f, 0.5f};
    math::Vec2f   margin;
    math::Vec2f   padding;
    bool          visible{true};
    bool          enabled{true};
    bool          touch_target{true};
    math::Color   color{1.0f, 1.0f, 1.0f, 1.0f};
    // interaction state
    bool          hovered{false};
    bool          pressed{false};
    bool          toggled{false};       ///< toggle/checkbox state
    float         value{0.0f};          ///< slider/progress 0..1
    float         min_value{0.0f};
    float         max_value{1.0f};
    std::vector<std::shared_ptr<Widget>> children;

    using ClickFn = std::function<void(Widget&)>;
    using ChangeFn = std::function<void(Widget&, float)>;
    ClickFn  on_click;  ///< wired by game/script code (never a no-op button)
    ChangeFn on_change; ///< slider/toggle value changes

    /// Helper: set value clamped to [min,max] and fire on_change.
    void set_value(float v) {
        value = std::clamp(v, min_value, max_value);
        if (on_change) on_change(*this, value);
    }
};

/// Concrete widget factories. Each returns a shared_ptr ready to add to a parent.
inline std::shared_ptr<Widget> make_panel(std::string id, math::Rectf rect) {
    auto w = std::make_shared<Widget>();
    w->type = WidgetType::Panel; w->id = std::move(id); w->rect = rect;
    w->color = math::Color{0.2f, 0.2f, 0.25f, 1.0f};
    return w;
}
inline std::shared_ptr<Widget> make_button(std::string id, std::string text, math::Rectf rect,
                                           Widget::ClickFn on_click = {}) {
    auto w = std::make_shared<Widget>();
    w->type = WidgetType::Button; w->id = std::move(id); w->text = std::move(text); w->rect = rect;
    w->anchor = Anchor{0,0,0,0}; // fixed-size, local rect respected
    w->color = math::Color{0.3f, 0.5f, 0.8f, 1.0f};
    w->on_click = std::move(on_click);
    return w;
}
inline std::shared_ptr<Widget> make_label(std::string id, std::string text, math::Rectf rect) {
    auto w = std::make_shared<Widget>();
    w->type = WidgetType::Label; w->id = std::move(id); w->text = std::move(text); w->rect = rect;
    w->anchor = Anchor{0,0,0,0};
    w->touch_target = false;
    w->color = math::Color{1.0f, 1.0f, 1.0f, 1.0f};
    return w;
}
inline std::shared_ptr<Widget> make_image(std::string id, std::string asset, math::Rectf rect) {
    auto w = std::make_shared<Widget>();
    w->type = WidgetType::Image; w->id = std::move(id); w->text = std::move(asset); w->rect = rect;
    w->anchor = Anchor{0,0,0,0};
    return w;
}
inline std::shared_ptr<Widget> make_slider(std::string id, math::Rectf rect, float min_v = 0.0f,
                                           float max_v = 1.0f, float v = 0.0f) {
    auto w = std::make_shared<Widget>();
    w->type = WidgetType::Slider; w->id = std::move(id); w->rect = rect;
    w->anchor = Anchor{0,0,0,0};
    w->min_value = min_v; w->max_value = max_v; w->value = std::clamp(v, min_v, max_v);
    return w;
}
inline std::shared_ptr<Widget> make_toggle(std::string id, std::string text, math::Rectf rect,
                                           bool on = false) {
    auto w = std::make_shared<Widget>();
    w->type = WidgetType::Toggle; w->id = std::move(id); w->text = std::move(text); w->rect = rect;
    w->anchor = Anchor{0,0,0,0};
    w->toggled = on; w->value = on ? 1.0f : 0.0f;
    return w;
}
inline std::shared_ptr<Widget> make_progress(std::string id, math::Rectf rect, float v = 0.0f) {
    auto w = std::make_shared<Widget>();
    w->type = WidgetType::ProgressBar; w->id = std::move(id); w->rect = rect;
    w->anchor = Anchor{0,0,0,0};
    w->value = std::clamp(v, 0.0f, 1.0f);
    return w;
}
inline std::shared_ptr<Widget> make_input(std::string id, std::string initial, math::Rectf rect) {
    auto w = std::make_shared<Widget>();
    w->type = WidgetType::InputField; w->id = std::move(id); w->text = std::move(initial); w->rect = rect;
    w->anchor = Anchor{0,0,0,0};
    return w;
}

/// The root UI canvas. Owns the widget tree and dispatches touch hits.
class Canvas {
public:
    Canvas() : root_{std::make_shared<Widget>()} {
        root_->type = WidgetType::Panel;
        root_->id = "root";
    }

    Widget& root() noexcept { return *root_; }

    /// Hit-test a touch in canvas coordinates; returns the deepest touchable
    /// widget under the point, or nullptr. Requires layout_pass() to have run.
    Widget* hit_test(math::Vec2f point);

    /// Add a child widget to a parent. Returns a reference to the new widget.
    Widget& add(Widget& parent, std::shared_ptr<Widget> child);

    /// Compute absolute rects for the whole tree from the root's rect. Call
    /// once per frame (or when the canvas resizes) before hit_test/render.
    void layout_pass();

    /// Dispatch a touch-down at `point`. Returns the widget that captured the
    /// touch (and fires on_click for buttons/toggles) or nullptr.
    Widget* touch_down(math::Vec2f point);
    /// Dispatch a touch-up at `point`. Completes a press/drag for sliders.
    void touch_up(math::Vec2f point);

    std::size_t widget_count() const noexcept;

private:
    std::shared_ptr<Widget> root_;
    std::size_t count_{1};
    Widget* captured_{nullptr};

    Widget* hit_test_impl(Widget& w, math::Vec2f p);
    void    layout_impl(Widget& w, const math::Rectf& parent_abs);
};

} // namespace vengine::ui
