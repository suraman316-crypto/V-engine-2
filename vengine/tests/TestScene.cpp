#include <vengine/scene/Scene.hpp>
#include <vengine/components/Components.hpp>
#include <vengine/serialization/SceneSerializer.hpp>

#include <gtest/gtest.h>

using namespace vengine;

TEST(Scene, CreateAndDestroyEntity) {
    scene::Scene s{"test"};
    auto e = s.create_entity("Player");
    ASSERT_TRUE(e.valid());
    EXPECT_EQ(s.entity_count(), 1u);
    s.destroy_entity(e);
    EXPECT_EQ(s.entity_count(), 0u);
}

TEST(Scene, StaleHandleDetectedAfterDestroy) {
    scene::Scene s;
    auto e = s.create_entity("X");
    s.destroy_entity(e);
    EXPECT_FALSE(s.registry().alive(e));
    EXPECT_EQ(s.registry().get<components::TransformComponent>(e), nullptr);
}

TEST(Scene, AddGetComponent) {
    scene::Scene s;
    auto e = s.create_entity("Sprite");
    auto& tc = s.registry().add<components::TransformComponent>(e);
    tc.value.position = {5, 6};
    auto* got = s.registry().get<components::TransformComponent>(e);
    ASSERT_NE(got, nullptr);
    EXPECT_FLOAT_EQ(got->value.position.x, 5.0f);
    EXPECT_TRUE(s.registry().has<components::TransformComponent>(e));
    s.registry().remove<components::TransformComponent>(e);
    EXPECT_FALSE(s.registry().has<components::TransformComponent>(e));
}

TEST(Scene, ViewWithMultipleComponents) {
    scene::Scene s;
    auto e1 = s.create_entity("a");
    auto e2 = s.create_entity("b");
    s.registry().add<components::TransformComponent>(e1);
    s.registry().add<components::TransformComponent>(e2);
    s.registry().add<components::SpriteRenderer>(e1);

    int both = 0, only_t = 0;
    s.registry().view<components::TransformComponent, components::SpriteRenderer>(
        [&](scene::Entity, const components::TransformComponent&, const components::SpriteRenderer&) {
            ++both;
        });
    s.registry().view<components::TransformComponent>(
        [&](scene::Entity, const components::TransformComponent&) { ++only_t; });
    EXPECT_EQ(both, 1);
    EXPECT_EQ(only_t, 2);
}

TEST(SceneSerializer, RoundTrip) {
    scene::Scene s{"Level01"};
    auto e = s.create_entity("Player");
    auto& tc = s.registry().add<components::TransformComponent>(e);
    tc.value.position = {12.5f, -7.0f};
    tc.value.rotation = 1.5f;
    tc.value.scale = {2.0f, 3.0f};
    auto& spr = s.registry().add<components::SpriteRenderer>(e);
    spr.texture_asset = "player.png";
    spr.flip_x = true;
    spr.layer = 2;
    auto& rb = s.registry().add<components::Rigidbody2D>(e);
    rb.type = components::BodyType::Kinematic;
    rb.fixed_rotation = true;
    auto& bc = s.registry().add<components::BoxCollider>(e);
    bc.size = {1.5f, 2.5f};
    bc.is_trigger = true;

    std::string json = serialization::serialize_scene(s);

    scene::Scene loaded{"loaded"};
    auto res = serialization::deserialize_scene(json, loaded);
    ASSERT_TRUE(res.ok()) << res.error().format();
    EXPECT_EQ(loaded.entity_count(), 1u);

    // Find the entity again by re-walking the transform storage.
    loaded.registry().view<components::TransformComponent>(
        [&](scene::Entity, const components::TransformComponent& t) {
            EXPECT_FLOAT_EQ(t.value.position.x, 12.5f);
            EXPECT_FLOAT_EQ(t.value.position.y, -7.0f);
            EXPECT_FLOAT_EQ(t.value.rotation, 1.5f);
            EXPECT_FLOAT_EQ(t.value.scale.x, 2.0f);
            EXPECT_FLOAT_EQ(t.value.scale.y, 3.0f);
        });
}

TEST(SceneSerializer, RejectsInvalidScene) {
    scene::Scene s;
    auto res = serialization::deserialize_scene("not json at all", s);
    ASSERT_FALSE(res.ok());
    EXPECT_EQ(res.error().code, core::ErrorCode::DeserializationFailed);

    auto res2 = serialization::deserialize_scene(R"({"type":"other","entities":[]})", s);
    ASSERT_FALSE(res2.ok());
    EXPECT_EQ(res2.error().code, core::ErrorCode::InvalidScene);
}
