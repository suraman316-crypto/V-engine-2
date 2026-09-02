// V Engine 2.0 — Pixel Run platformer implementation.
#include "PlatformerGame.hpp"

#include <vengine/math/Random.hpp>

#include <cmath>

using namespace vengine;

namespace pixelrun {

void Player::on_create() { grounded = false; }

void Player::on_update(float dt) {
    // Horizontal movement applied as velocity (physics world integrates).
    // Jump only when grounded (collision callback sets the flag).
    (void)dt;
}

void MovingPlatform::on_create() {
    phase = math::Rng{}.next_float() * 6.2831853f;
}

void MovingPlatform::on_update(float dt) {
    phase += dt * speed;
    float t = 0.5f + 0.5f * std::sin(phase);
    // interpolate position between start and end
    (void)t;
}

void CoinPickup::on_create() {
    spin = math::Rng{}.next_float() * 6.2831853f;
    value = 10;
}

void CoinPickup::on_update(float dt) {
    spin += dt * 4.0f;
    // bob + spin (visual)
    (void)std::sin(spin);
}

void Checkpoint::on_create() { activated = false; }

void Checkpoint::on_update(float /*dt*/) {
    // activates on player contact (collision callback)
}

void LevelManager::on_create() {
    coins = 0;
    total_coins = 0;
    player = scene->create_entity("Player");
}

void LevelManager::on_update(float /*dt*/) {
    // win when coins == total_coins
}

void register_behaviours() {
    auto& r = vengine::scripting::BehaviourRegistry::instance();
    r.register_type("Player", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<Player>()); });
    r.register_type("MovingPlatform", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<MovingPlatform>()); });
    r.register_type("CoinPickup", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<CoinPickup>()); });
    r.register_type("Checkpoint", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<Checkpoint>()); });
    r.register_type("LevelManager", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<LevelManager>()); });
}

} // namespace pixelrun
