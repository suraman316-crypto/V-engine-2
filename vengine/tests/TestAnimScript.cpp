#include <vengine/animation/Tween.hpp>
#include <vengine/components/Components.hpp>
#include <vengine/math/Easing.hpp>
#include <vengine/scene/Scene.hpp>
#include <vengine/scripting/NativeScript.hpp>

#include <gtest/gtest.h>

using namespace vengine;
using namespace vengine::animation;
using namespace vengine::scene;
using namespace vengine::scripting;

// ---------------------------------------------------------------------------
// Tween
// ---------------------------------------------------------------------------

TEST(Tween, InterpolatesFromToOverTime) {
    float value = 0.0f;
    TweenManager mgr;
    mgr.add(Tween{[&](float v){ value = v; }, 0.0f, 10.0f, 1.0f, 0.0f, math::ease_linear});
    mgr.update(0.5f);
    EXPECT_NEAR(value, 5.0f, 1e-3f);
    mgr.update(0.5f);
    EXPECT_NEAR(value, 10.0f, 1e-3f);
    EXPECT_EQ(mgr.count(), 0u); // completed & retired
}

TEST(Tween, EasingQuadCurves) {
    float value = 0.0f;
    TweenManager mgr;
    mgr.add(Tween{[&](float v){ value = v; }, 0.0f, 1.0f, 1.0f, 0.0f, math::ease_in_quad});
    mgr.update(0.5f);
    // ease_in_quad(0.5) = 0.25
    EXPECT_NEAR(value, 0.25f, 1e-3f);
}

TEST(Tween, Vec2TweenLerps) {
    math::Vec2f v{0,0};
    TweenManager mgr;
    mgr.add(TweenVec2{[&](const math::Vec2f& p){ v = p; }, {0,0}, {10,20}, 1.0f});
    mgr.update(0.5f);
    EXPECT_NEAR(v.x, 5.0f, 1e-3f);
    EXPECT_NEAR(v.y, 10.0f, 1e-3f);
}

TEST(Tween, LoopKeepsAlive) {
    float value = 0.0f;
    TweenManager mgr;
    mgr.add(Tween{[&](float v){ value = v; }, 0.0f, 1.0f, 0.5f, 0.0f, math::ease_linear, true});
    mgr.update(0.5f); // completes one loop
    EXPECT_EQ(mgr.count(), 1u); // still alive
    mgr.update(0.5f); // loops again
    EXPECT_EQ(mgr.count(), 1u);
}

TEST(Tween, CompleteCallbackFires) {
    bool done = false;
    TweenManager mgr;
    mgr.add(Tween{[&](float){}, 0.0f, 1.0f, 0.1f, 0.0f, math::ease_linear,
                  false, false, true, false, nullptr, [&]{ done = true; }});
    mgr.update(0.2f);
    EXPECT_TRUE(done);
}

// ---------------------------------------------------------------------------
// NativeScript
// ---------------------------------------------------------------------------

namespace {

class Counter : public ScriptBehaviour {
public:
    int creates{0}, updates{0}, destroys{0};
    void on_create() override { ++creates; }
    void on_update(float) override { ++updates; }
    void on_destroy() override { ++destroys; }
};

} // namespace

TEST(NativeScript, LifecycleCreateThenUpdate) {
    Scene s;
    NativeScriptRuntime rt;
    auto e = s.create_entity();
    auto b = std::make_unique<Counter>();
    Counter* raw = b.get();
    rt.attach(e, std::move(b));
    EXPECT_EQ(raw->creates, 0);
    rt.tick(s, 0.016f);
    EXPECT_EQ(raw->creates, 1);
    rt.tick(s, 0.016f);
    EXPECT_EQ(raw->updates, 2);
    EXPECT_EQ(rt.instance_count(), 1u);
}

TEST(NativeScript, RegistryFactory) {
    BehaviourRegistry::instance().register_type("Counter",
        []{ return std::unique_ptr<ScriptBehaviour>(std::make_unique<Counter>()); });
    Scene s;
    NativeScriptRuntime rt;
    auto e = s.create_entity();
    EXPECT_TRUE(rt.attach(e, "Counter"));
    rt.tick(s, 0.016f);
    EXPECT_EQ(rt.instance_count(), 1u);
}

TEST(NativeScript, UnknownTypeReturnsFalse) {
    Scene s;
    NativeScriptRuntime rt;
    auto e = s.create_entity();
    EXPECT_FALSE(rt.attach(e, "DoesNotExist"));
}
