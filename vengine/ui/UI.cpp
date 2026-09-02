#include <vengine/ui/UI.hpp>

#include <algorithm>

namespace vengine::ui {

Widget& Canvas::add(Widget& parent, std::shared_ptr<Widget> child) {
    parent.children.push_back(std::move(child));
    ++count_;
    return *parent.children.back();
}

std::size_t Canvas::widget_count() const noexcept { return count_; }

void Canvas::layout_pass() {
    layout_impl(*root_, root_->rect);
}

void Canvas::layout_impl(Widget& w, const math::Rectf& parent_abs) {
    // Anchor stretches the widget within the parent; otherwise use local rect.
    float ax = parent_abs.x + w.rect.x;
    float ay = parent_abs.y + w.rect.y;
    float aw = w.rect.width();
    float ah = w.rect.height();
    // Stretched anchor (min != max) fills parent in that axis.
    if (w.anchor.min_x != w.anchor.max_x) {
        ax = parent_abs.x + parent_abs.width() * w.anchor.min_x + w.margin.x;
        aw = parent_abs.width() * (w.anchor.max_x - w.anchor.min_x) - w.margin.x * 2.0f;
    }
    if (w.anchor.min_y != w.anchor.max_y) {
        ay = parent_abs.y + parent_abs.height() * w.anchor.min_y + w.margin.y;
        ah = parent_abs.height() * (w.anchor.max_y - w.anchor.min_y) - w.margin.y * 2.0f;
    }
    w.absolute = math::Rectf{ax, ay, aw, ah};
    for (auto& c : w.children) if (c) layout_impl(*c, w.absolute);
}

Widget* Canvas::hit_test(math::Vec2f point) {
    return hit_test_impl(*root_, point);
}

Widget* Canvas::hit_test_impl(Widget& w, math::Vec2f p) {
    if (!w.visible || !w.enabled) return nullptr;
    if (w.touch_target && w.absolute.contains(p)) {
        for (auto& child : w.children) {
            if (child) if (Widget* hit = hit_test_impl(*child, p)) return hit;
        }
        return &w;
    }
    for (auto& child : w.children) {
        if (child) if (Widget* hit = hit_test_impl(*child, p)) return hit;
    }
    return nullptr;
}

Widget* Canvas::touch_down(math::Vec2f point) {
    Widget* w = hit_test(point);
    if (!w) return nullptr;
    captured_ = w;
    w->pressed = true;
    w->hovered = true;
    switch (w->type) {
    case WidgetType::Button:
        // click fires on touch_up for proper press-and-cancel semantics.
        break;
    case WidgetType::Slider: {
        float t = std::clamp((point.x - w->absolute.x) / w->absolute.width(), 0.0f, 1.0f);
        w->set_value(w->min_value + (w->max_value - w->min_value) * t);
        break;
    }
    default: break;
    }
    return w;
}

void Canvas::touch_up(math::Vec2f point) {
    if (!captured_) return;
    Widget* w = captured_;
    captured_ = nullptr;
    bool over = w->absolute.contains(point);
    if (w->type == WidgetType::Button) {
        if (over && w->on_click) w->on_click(*w);
    } else if (w->type == WidgetType::Toggle || w->type == WidgetType::Checkbox) {
        if (over) {
            w->toggled = !w->toggled;
            w->value = w->toggled ? 1.0f : 0.0f;
            if (w->on_change) w->on_change(*w, w->value);
        }
    }
    w->pressed = false;
    w->hovered = over;
}

} // namespace vengine::ui
