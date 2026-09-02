#pragma once

// V Engine 2.0 — Sample game 2: "Pixel Run" platformer.
// Demonstrates side-scroller physics, tile collision, pickups, parallax.

#include <vengine/Engine.hpp>
#include <vengine/scripting/NativeScript.hpp>

namespace pixelrun {

class Player : public vengine::scripting::ScriptBehaviour {
public:
    float move_speed{240.0f};
    float jump_velocity{520.0f};
    bool grounded{false};
    void on_create() override;
    void on_update(float dt) override;
};

class MovingPlatform : public vengine::scripting::ScriptBehaviour {
public:
    vengine::math::Vec2f start;
    vengine::math::Vec2f end;
    float phase{0.0f};
    float speed{1.0f};
    void on_create() override;
    void on_update(float dt) override;
};

class CoinPickup : public vengine::scripting::ScriptBehaviour {
public:
    int value{10};
    float spin{0.0f};
    void on_create() override;
    void on_update(float dt) override;
};

class Checkpoint : public vengine::scripting::ScriptBehaviour {
public:
    vengine::math::Vec2f respawn;
    bool activated{false};
    void on_create() override;
    void on_update(float dt) override;
};

class LevelManager : public vengine::scripting::ScriptBehaviour {
public:
    int coins{0};
    int total_coins{0};
    vengine::scene::Entity player{};
    void on_create() override;
    void on_update(float dt) override;
};

void register_behaviours();

} // namespace pixelrun
