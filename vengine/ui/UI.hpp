#pragma once

#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/math/Rect.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/math/Color.hpp>

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
    Anchor        anchor;
    math::Vec2f   pivot{0.5f, 0.5f};
    math::Vec2f   margin;
    math::Vec2f   padding;
    bool          visible{true};
    bool          enabled{true};
    bool          touch_target{true};
    math::Color   color{1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<std::shared_ptr<Widget>> children;

    using ClickFn = std::function<void(Widget&)>;
    ClickFn on_click; ///< wired by game/script code (never a no-op button)
};

/// The root UI canvas. Owns the widget tree and dispatches touch hits.
class Canvas {
public:
    Canvas() : root_{std::make_shared<Widget>()} {
        root_->type = WidgetType::Panel;
        root_->id = "root";
    }

    Widget& root() noexcept { return *root_; }

    /// Hit-test a touch in canvas coordinates; returns the deepest touchable
    /// widget under the point, or nullptr.
    Widget* hit_test(math::Vec2f point);

    /// Add a child widget to a parent. Returns a reference to the new widget.
    Widget& add(Widget& parent, std::shared_ptr<Widget> child);

    std::size_t widget_count() const noexcept;

private:
    std::shared_ptr<Widget> root_;
    std::size_t count_{1};

    Widget* hit_test_impl(Widget& w, math::Vec2f p);
};

} // namespace vengine::ui
