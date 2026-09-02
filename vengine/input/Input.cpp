#include <vengine/input/Input.hpp>
#include <vengine/core/Assert.hpp>

#include <algorithm>
#include <cmath>

namespace vengine::input {

Input::Input() {
    for (auto& p : pointers_) p.id = -1;
}

void Input::on_pointer_down(int id, math::Vec2f px, math::Vec2f normalized) {
    auto idx = pointer_index_for(id);
    if (idx >= kMaxPointers) {
        // Too many simultaneous touches; ignore the overflow gracefully.
        return;
    }
    Pointer& p = pointers_[idx];
    p.id = id;
    p.position = px;
    p.normalized = normalized;
    p.down = true;
    p.pressed_this_frame = true;
    p.released_this_frame = false;
}

void Input::on_pointer_up(int id) {
    auto idx = pointer_index_for(id);
    if (idx >= kMaxPointers) return;
    Pointer& p = pointers_[idx];
    if (p.id != id) return;
    p.down = false;
    p.released_this_frame = true;
}

void Input::on_pointer_move(int id, math::Vec2f px, math::Vec2f normalized) {
    auto idx = pointer_index_for(id);
    if (idx >= kMaxPointers) return;
    Pointer& p = pointers_[idx];
    if (p.id != id) return;
    p.position = px;
    p.normalized = normalized;
}

void Input::on_key_down(Key k) {
    auto i = static_cast<std::size_t>(k);
    if (i >= keys_.size()) return;
    auto& s = keys_[i];
    if (!s.down) s.pressed_this_frame = true;
    s.down = true;
}

void Input::on_key_up(Key k) {
    auto i = static_cast<std::size_t>(k);
    if (i >= keys_.size()) return;
    auto& s = keys_[i];
    s.down = false;
    s.released_this_frame = true;
}

void Input::map_action(std::string name, std::vector<Key> keys) {
    actions_[std::move(name)] = ActionBinding{std::move(keys)};
}

bool Input::is_action_pressed(std::string_view name) const {
    auto it = actions_.find(std::string(name));
    if (it == actions_.end()) return false;
    for (Key k : it->second.keys) {
        if (key_down(k)) return true;
    }
    return false;
}

bool Input::is_action_just_pressed(std::string_view name) const {
    auto it = actions_.find(std::string(name));
    if (it == actions_.end()) return false;
    for (Key k : it->second.keys) {
        if (key_just_pressed(k)) return true;
    }
    return false;
}

bool Input::is_action_just_released(std::string_view name) const {
    auto it = actions_.find(std::string(name));
    if (it == actions_.end()) return false;
    for (Key k : it->second.keys) {
        if (key_just_released(k)) return true;
    }
    return false;
}

void Input::map_axis(std::string name, Key negative, Key positive) {
    axes_[std::move(name)] = AxisBinding{negative, positive};
}

float Input::axis_value(std::string_view name) const {
    auto it = axes_.find(std::string(name));
    if (it == axes_.end()) return 0.0f;
    float v = 0.0f;
    if (key_down(it->second.positive)) v += 1.0f;
    if (key_down(it->second.negative)) v -= 1.0f;
    return v;
}

bool Input::key_down(Key k) const noexcept {
    auto i = static_cast<std::size_t>(k);
    return i < keys_.size() && keys_[i].down;
}
bool Input::key_just_pressed(Key k) const noexcept {
    auto i = static_cast<std::size_t>(k);
    return i < keys_.size() && keys_[i].pressed_this_frame;
}
bool Input::key_just_released(Key k) const noexcept {
    auto i = static_cast<std::size_t>(k);
    return i < keys_.size() && keys_[i].released_this_frame;
}

std::size_t Input::pointer_count() const noexcept {
    std::size_t n = 0;
    for (const auto& p : pointers_) if (p.id != -1 && p.down) ++n;
    return n;
}

const Pointer* Input::pointer(std::size_t i) const noexcept {
    if (i >= kMaxPointers) return nullptr;
    return pointers_[i].id == -1 ? nullptr : &pointers_[i];
}

const Pointer* Input::primary_pointer() const noexcept {
    for (const auto& p : pointers_) if (p.id != -1) return &p;
    return nullptr;
}

bool Input::any_pointer_down() const noexcept {
    for (const auto& p : pointers_) if (p.id != -1 && p.down) return true;
    return false;
}

float Input::pinch_distance() const noexcept {
    const Pointer* first = nullptr;
    const Pointer* second = nullptr;
    for (const auto& p : pointers_) {
        if (p.id == -1 || !p.down) continue;
        if (!first) first = &p;
        else if (!second) { second = &p; break; }
    }
    if (!first || !second) return 0.0f;
    float dx = second->position.x - first->position.x;
    float dy = second->position.y - first->position.y;
    return std::sqrt(dx * dx + dy * dy);
}

void Input::end_frame() {
    for (auto& p : pointers_) {
        p.pressed_this_frame = false;
        p.released_this_frame = false;
        // Reclaim slots whose pointers were released last frame.
        if (!p.down && p.released_this_frame) {
            // already reset; keep id until next up? We clear released ones now.
        }
        if (!p.down) {
            // Keep the slot available: only clear id when truly released.
            // To preserve "just released" within the same frame we defer
            // clearing to the next end_frame after a release.
        }
    }
    // Clear released pointers' ids so slots can be reused next frame.
    for (auto& p : pointers_) {
        if (!p.down && p.id != -1) p.id = -1;
    }
    for (auto& s : keys_) {
        s.pressed_this_frame = false;
        s.released_this_frame = false;
    }
}

std::size_t Input::pointer_index_for(int id) const noexcept {
    std::size_t free_slot = kMaxPointers;
    for (std::size_t i = 0; i < kMaxPointers; ++i) {
        if (pointers_[i].id == id) return i;
        if (pointers_[i].id == -1 && free_slot == kMaxPointers) free_slot = i;
    }
    return free_slot;
}

} // namespace vengine::input
