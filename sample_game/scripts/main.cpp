// V Engine 2.0 — Starfall Tactics: minimal headless main.
// Registers the game's behaviours and runs a few frames to prove the
// engine + sample wiring boots end-to-end.
#include "GameBehaviours.hpp"

#include <vengine/EngineV2.hpp>

#include <gtest/gtest.h>

using namespace vengine;

TEST(Starfall, RegistersBehavioursAndBoots) {
    starfall::register_behaviours();
    auto& reg = scripting::BehaviourRegistry::instance();
    EXPECT_NE(reg.find("PlayerController"), nullptr);
    EXPECT_NE(reg.find("EnemyController"), nullptr);
    EXPECT_NE(reg.find("GameManager"), nullptr);

    EngineV2 e;
    ASSERT_TRUE(e.boot("sample_game/game.json").ok());
    for (int i = 0; i < 5; ++i) e.frame(0.016f);
    EXPECT_GT(e.frame_stats().history_size(), 0u);
    e.shutdown();
}
