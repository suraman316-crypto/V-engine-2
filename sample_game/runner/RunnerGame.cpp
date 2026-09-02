// V Engine 2.0 — Neon Runner endless runner implementation.
#include "RunnerGame.hpp"

#include <vengine/math/Random.hpp>

using namespace vengine;

namespace neonrun {

void Runner::on_create() { grounded = false; sliding = false; }

void Runner::on_update(float dt) {
    // forward motion is implied by world scroll; jump/slide on input
    (void)dt;
}

void ObstacleSpawner::on_create() {
    spawn_timer = spawn_interval;
    difficulty = 1.0f;
}

void ObstacleSpawner::on_update(float dt) {
    spawn_timer -= dt;
    if (spawn_timer <= 0.0f) {
        spawn_timer = spawn_interval / difficulty;
        difficulty += 0.02f;  // ramp up
        // spawn obstacle entity here
    }
}

void ParallaxLayer::on_create() { scroll_speed = 100.0f; }

void ParallaxLayer::on_update(float dt) {
    // offset texture coords by scroll_speed * parallax_factor * dt
    (void)dt;
}

void Coin::on_create() {}

void Coin::on_update(float /*dt*/) {
    // collected on contact with runner
}

void GameManager::on_create() {
    score = 0; coins = 0; distance = 0.0f; speed_multiplier = 1.0f;
}

void GameManager::on_update(float dt) {
    speed_multiplier += dt * 0.05f;  // gradually faster
}

void register_behaviours() {
    auto& r = vengine::scripting::BehaviourRegistry::instance();
    r.register_type("Runner", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<Runner>()); });
    r.register_type("ObstacleSpawner", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<ObstacleSpawner>()); });
    r.register_type("ParallaxLayer", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<ParallaxLayer>()); });
    r.register_type("Coin", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<Coin>()); });
    r.register_type("GameManager", []{ return std::unique_ptr<vengine::scripting::ScriptBehaviour>(std::make_unique<GameManager>()); });
}

} // namespace neonrun
