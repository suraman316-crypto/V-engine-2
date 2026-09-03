#pragma once

// V Engine 2.0 — Sample game: "Starfall Tactics"
// A top-down 2D space shooter demonstrating the full engine stack:
// physics, rendering, audio, particles, UI, scripting, AI, animation.
//
// This file declares the game's behaviours. Implementation is gameplay-only;
// it uses engine APIs (no engine internals).

#include <vengine/Engine.hpp>
#include <vengine/ai/BehaviorTree.hpp>
#include <vengine/ai/Navigation.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/scripting/NativeScript.hpp>

namespace starfall {

/// Player ship: keyboard/touch movement, fires projectiles toward aim.
class PlayerController : public vengine::scripting::ScriptBehaviour {
public:
    float speed{320.0f};
    float fire_rate{8.0f};          ///< shots per second
    float fire_cooldown{0.0f};
    vengine::math::Vec2f aim;

    void on_create() override;
    void on_update(float dt) override;
};

/// Enemy ship: seeks the player, fires occasionally, dies in one hit.
class EnemyController : public vengine::scripting::ScriptBehaviour {
public:
    float speed{140.0f};
    float fire_cooldown{2.0f};
    float health{1.0f};
    vengine::scene::Entity player{};

    void on_create() override;
    void on_update(float dt) override;
};

/// Projectile: travels in a straight line, despawns off-screen or on hit.
class Projectile : public vengine::scripting::ScriptBehaviour {
public:
    vengine::math::Vec2f velocity{};
    float lifetime{3.0f};
    bool from_player{true};

    void on_create() override;
    void on_update(float dt) override;
};

/// Pickup: floating star the player can collect for score.
class Pickup : public vengine::scripting::ScriptBehaviour {
public:
    float bob_phase{0.0f};
    void on_create() override;
    void on_update(float dt) override;
};

/// Game manager: spawns waves, tracks score, shows the HUD.
class GameManager : public vengine::scripting::ScriptBehaviour {
public:
    int score{0};
    int wave{1};
    float spawn_timer{0.0f};
    vengine::scene::Entity player{};

    void on_create() override;
    void on_update(float dt) override;
};

/// Register all game behaviours with the engine's BehaviourRegistry.
void register_behaviours();

} // namespace starfall
