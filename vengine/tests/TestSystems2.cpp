#include <vengine/scene/Scene.hpp>
#include <vengine/scene/Systems.hpp>
#include <vengine/components/Components.hpp>

#include <gtest/gtest.h>

using namespace vengine;
using namespace vengine::math;
using namespace vengine::scene;
using namespace vengine::components;

namespace {
struct HitEvent { scene::Entity a; scene::Entity b; };
} // namespace

TEST(Systems, FunctionSystemRunsInUpdatePhase) {
    Scene s;
    int counter = 0;
    s.add_system([&](Scene&, float) { ++counter; });
    s.on_create();
    s.update(0.016f);
    EXPECT_EQ(counter, 1);
}

TEST(Systems, FixedUpdateSeparatesFromUpdate) {
    Scene s;
    int updates = 0, fixed = 0;
    s.attach_system(std::make_shared<FunctionSystem>(UpdatePhase::Update, 0,
        [&](Scene&, float) { ++updates; }));
    s.attach_system(std::make_shared<FunctionSystem>(UpdatePhase::FixedUpdate, 0,
        [&](Scene&, float) { ++fixed; }));
    s.on_create();
    s.fixed_update(0.02f);
    s.update(0.016f);
    EXPECT_EQ(updates, 1);
    EXPECT_EQ(fixed, 1);
}

TEST(Systems, PriorityOrdersWithinPhase) {
    Scene s;
    std::vector<int> order;
    s.attach_system(std::make_shared<FunctionSystem>(UpdatePhase::Update, 10,
        [&](Scene&, float) { order.push_back(10); }));
    s.attach_system(std::make_shared<FunctionSystem>(UpdatePhase::Update, 1,
        [&](Scene&, float) { order.push_back(1); }));
    s.attach_system(std::make_shared<FunctionSystem>(UpdatePhase::Update, 5,
        [&](Scene&, float) { order.push_back(5); }));
    s.on_create();
    s.update(0.016f);
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 5);
    EXPECT_EQ(order[2], 10);
}

TEST(EventBus, PublishReachesSubscribers) {
    EventBus bus;
    int hits = 0;
    auto sub = bus.subscribe<HitEvent>([&](const HitEvent&) { ++hits; });
    HitEvent e{};
    bus.publish(e);
    bus.publish(e);
    EXPECT_EQ(hits, 2);
    sub.reset();
    bus.publish(e);
    EXPECT_EQ(hits, 2); // unsubscribed
}

TEST(EventBus, MidDispatchUnsubscribeDoesNotAffectCurrentDispatch) {
    // Safe semantics: we snapshot subscribers before dispatch, so unsubscribing
    // during a dispatch must NOT remove a callback from the in-flight batch.
    EventBus bus;
    int hits = 0;
    std::shared_ptr<Subscription> inner;
    auto outer = bus.subscribe<HitEvent>([&](const HitEvent&) {
        ++hits;
        if (inner) inner->reset(); // unsubscribe inner mid-dispatch
    });
    inner = std::make_shared<Subscription>(bus.subscribe<HitEvent>([&](const HitEvent&) {
        ++hits;
    }));
    HitEvent e{};
    bus.publish(e); // both fire this round (snapshot already taken)
    EXPECT_EQ(hits, 2);
    hits = 0;
    bus.publish(e); // inner was unsubscribed, only outer fires
    EXPECT_EQ(hits, 1);
}

TEST(Systems, SystemCanIterateComponents) {
    Scene s;
    auto e = s.create_entity();
    s.registry().add<TransformComponent>(e, Transform2D{Vec2f{0,0}, 0.0f, Vec2f{1,1}});
    int seen = 0;
    s.add_system([&](Scene& sc, float) {
        sc.registry().view<TransformComponent>([&](Entity, const TransformComponent&) {
            ++seen;
        });
    });
    s.on_create();
    s.update(0.016f);
    EXPECT_EQ(seen, 1);
}
