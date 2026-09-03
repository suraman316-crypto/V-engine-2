#pragma once

#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/math/Vec2.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vengine::input {

/// Source device that produced an input event.
enum class Source : std::uint8_t {
    Touch,
    Mouse,
    Keyboard,
    Gamepad,
};

/// A single pointer (touch/mouse) state. Supports multitouch up to kMaxPointers.
struct Pointer {
    int        id{-1};          ///< touch id, or 0 for mouse
    math::Vec2f position{0.0f, 0.0f};   ///< screen pixels
    math::Vec2f normalized{0.0f, 0.0f}; ///< 0..1 across the surface
    bool       down{false};
    bool       pressed_this_frame{false};
    bool       released_this_frame{false};
};

/// Maximum simultaneous pointers (fingers) tracked.
inline constexpr std::size_t kMaxPointers = 10;

/// Key/button codes for keyboard & gamepad. A minimal but real set.
enum class Key : std::uint16_t {
    Unknown = 0,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Space, Enter, Escape, Backspace, Tab,
    Up, Down, Left, Right,
    Shift, Ctrl, Alt,
    GamepadA, GamepadB, GamepadX, GamepadY,
    GamepadLeft, GamepadRight, GamepadUp, GamepadDown,
    GamepadStart, GamepadBack,
    Count
};

/// The Input facade. The engine pumps raw platform events into it each frame
/// via on_pointer_*/on_key; game code queries high-level actions.
///
/// Thread safety: not thread-safe; events are pumped on the main thread and
/// queried during update(). The end_frame() call flips "just pressed/released"
/// edge bits so a query in one frame only reflects that frame's events.
class Input {
public:
    Input();

    // ---- Raw event pump (called by the platform layer) ---------------------
    void on_pointer_down(int id, math::Vec2f px, math::Vec2f normalized);
    void on_pointer_up(int id);
    void on_pointer_move(int id, math::Vec2f px, math::Vec2f normalized);
    void on_key_down(Key k);
    void on_key_up(Key k);

    // ---- Action mapping (the README Input API) -----------------------------
    /// Bind a logical action name to one or more keys. Re-binding replaces.
    void map_action(std::string name, std::vector<Key> keys);
    bool is_action_pressed(std::string_view name) const;
    bool is_action_just_pressed(std::string_view name) const;
    bool is_action_just_released(std::string_view name) const;

    /// Bind an axis (e.g. "Horizontal") to left/right keys; value in [-1, 1].
    void map_axis(std::string name, Key negative, Key positive);
    float axis_value(std::string_view name) const;

    // ---- Direct queries -----------------------------------------------------
    bool key_down(Key k) const noexcept;
    bool key_just_pressed(Key k) const noexcept;
    bool key_just_released(Key k) const noexcept;

    std::size_t pointer_count() const noexcept;       ///< currently down
    const Pointer* pointer(std::size_t i) const noexcept;
    const Pointer* primary_pointer() const noexcept;  ///< first active touch/mouse
    bool           any_pointer_down() const noexcept;

    /// Pinch distance between the first two active pointers (0 if < 2).
    float pinch_distance() const noexcept;

    // ---- Frame bookkeeping --------------------------------------------------
    void end_frame();

private:
    struct KeyState {
        bool down{false};
        bool pressed_this_frame{false};
        bool released_this_frame{false};
    };

    struct ActionBinding {
        std::vector<Key> keys;
    };
    struct AxisBinding {
        Key negative{Key::Unknown};
        Key positive{Key::Unknown};
    };

    Pointer find_or_create(int id);
    std::size_t pointer_index_for(int id) const noexcept;

    std::array<Pointer, kMaxPointers> pointers_{};
    std::array<KeyState, static_cast<std::size_t>(Key::Count)> keys_{};

    std::unordered_map<std::string, ActionBinding> actions_;
    std::unordered_map<std::string, AxisBinding>   axes_;
};

} // namespace vengine::input
