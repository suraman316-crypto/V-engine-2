#include <vengine/ui/UI.hpp>

namespace vengine::ui {

Widget& Canvas::add(Widget& parent, std::shared_ptr<Widget> child) {
    parent.children.push_back(std::move(child));
    ++count_;
    return *parent.children.back();
}

std::size_t Canvas::widget_count() const noexcept { return count_; }

Widget* Canvas::hit_test(math::Vec2f point) {
    return hit_test_impl(*root_, point);
}

Widget* Canvas::hit_test_impl(Widget& w, math::Vec2f p) {
    if (!w.visible || !w.enabled) return nullptr;
    // Point is in the widget's local space relative to its parent; here we
    // approximate hit testing against the widget's rect directly (the layout
    // pass, added in a later phase, will compute absolute rects first).
    if (w.touch_target && w.rect.contains(p)) {
        // Recurse children first so the deepest widget wins.
        for (auto& child : w.children) {
            if (child) {
                math::Vec2f local{p.x - w.rect.x, p.y - w.rect.y};
                if (Widget* hit = hit_test_impl(*child, local)) return hit;
            }
        }
        return &w;
    }
    for (auto& child : w.children) {
        if (child) {
            math::Vec2f local{p.x - w.rect.x, p.y - w.rect.y};
            if (Widget* hit = hit_test_impl(*child, local)) return hit;
        }
    }
    return nullptr;
}

} // namespace vengine::ui
