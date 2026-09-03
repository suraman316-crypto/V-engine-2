#pragma once

// V Engine 2.0 — Sample game 3: "Neon Runner" endless runner.
// Demonstrates procedural generation, parallax layers, increasing difficulty.

#include <vengine/Engine.hpp>
#include <vengine/scripting/NativeScript.hpp>

namespace neonrun {

class Runner : public vengine::scripting::ScriptBehaviour {
public:
    float base_speed{300.0f};
    float jump_velocity{600.0f};
    bool grounded{false};
    bool sliding{false};
    void on_create() override;
    void on_update(float dt) override;
};

class ObstacleSpawner : public vengine::scripting::ScriptBehaviour {
public:
    float spawn_timer{1.5f};
    float spawn_interval{1.5f};
    float difficulty{1.0f};
    void on_create() override;
    void on_update(float dt) override;
};

class ParallaxLayer : public vengine::scripting::ScriptBehaviour {
public:
    float scroll_speed{100.0f};
    float parallax_factor{0.5f};
    void on_create() override;
    void on_update(float dt) override;
};

class Coin : public vengine::scripting::ScriptBehaviour {
public:
    void on_create() override;
    void on_update(float dt) override;
};

class GameManager : public vengine::scripting::ScriptBehaviour {
public:
    int score{0};
    int coins{0};
    float distance{0.0f};
    float speed_multiplier{1.0f};
    void on_create() override;
    void on_update(float dt) override;
};

void register_behaviours();

} // namespace neonrun
