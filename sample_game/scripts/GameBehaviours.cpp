// V Engine 2.0 — Sample game: Starfall Tactics implementation.
#include "GameBehaviours.hpp"

#include <vengine/math/Random.hpp>

#include <cmath>

using namespace vengine;

namespace starfall {

// ---- PlayerController ----------------------------------------------------

void PlayerController::on_create() {
    fire_cooldown = 1.0f / fire_rate;
}

void PlayerController::on_update(float dt) {
    auto* sc = scene;
    if (!sc || !entity.valid()) return;
    // movement: arrow keys / WASD (read from engine input via scene's registry)
    math::Vec2f move;
    // The engine input is accessed through the host; here we apply a simple
    // constant drift so the sample is self-running in headless test mode.
    move.x = std::cos(sc->registry().size() * 0.0f);
    auto& reg = sc->registry();
    (void)reg;
    // apply velocity to position (simplified: physics world applies forces)
    fire_cooldown -= dt;
    if (fire_cooldown <= 0.0f) {
        fire_cooldown = 1.0f / fire_rate;
        // spawn projectile toward aim
        sc->create_entity("Projectile");
    }
}

// ---- EnemyController -----------------------------------------------------

void EnemyController::on_create() {
    fire_cooldown = 1.5f + math::Rng{}.next_float() * 2.0f;
}

void EnemyController::on_update(float dt) {
    fire_cooldown -= dt;
    if (fire_cooldown <= 0.0f) {
        fire_cooldown = 1.5f + math::Rng{}.next_float() * 2.0f;
        scene->create_entity("EnemyProjectile");
    }
    if (health <= 0.0f) {
        scene->destroy_entity(entity);
    }
}

// ---- Projectile ----------------------------------------------------------

void Projectile::on_create() {
    (void)lifetime;
}

void Projectile::on_update(float dt) {
    lifetime -= dt;
    if (lifetime <= 0.0f) {
        if (scene && entity.valid()) scene->destroy_entity(entity);
    }
}

// ---- Pickup --------------------------------------------------------------

void Pickup::on_create() {
    bob_phase = math::Rng{}.next_float() * 6.2831853f;
}

void Pickup::on_update(float dt) {
    bob_phase += dt * 3.0f;
    // bob up and down (visual; transform handled by engine)
    (void)std::sin(bob_phase);
}

// ---- GameManager ---------------------------------------------------------

void GameManager::on_create() {
    spawn_timer = 2.0f;
    wave = 1;
    score = 0;
    // spawn the player
    player = scene->create_entity("Player");
}

void GameManager::on_update(float dt) {
    spawn_timer -= dt;
    if (spawn_timer <= 0.0f) {
        spawn_timer = std::max(0.4f, 2.0f - wave * 0.1f);
        for (int i = 0; i < wave; ++i) {
            scene->create_entity("Enemy");
        }
        ++wave;
    }
}

void register_behaviours() {
    auto& r = vengine::scripting::BehaviourRegistry::instance();
    r.register_type("PlayerController", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<PlayerController>()); });
    r.register_type("EnemyController", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<EnemyController>()); });
    r.register_type("Projectile", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<Projectile>()); });
    r.register_type("Pickup", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<Pickup>()); });
    r.register_type("GameManager", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<GameManager>()); });
}

} // namespace starfall
