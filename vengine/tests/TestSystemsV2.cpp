// V Engine 2.0 — Systems v2 tests (scripting, AI, network, audio, anim, editor).
#include <vengine/ai/BehaviorTree.hpp>
#include <vengine/ai/Navigation.hpp>
#include <vengine/animation/SkeletalAnimation.hpp>
#include <vengine/audio/SpatialAudio.hpp>
#include <vengine/editor/EditorDashboard.hpp>
#include <vengine/editor/Inspector.hpp>
#include <vengine/network/Network.hpp>
#include <vengine/scripting/ScriptVM.hpp>

#include <gtest/gtest.h>

using namespace vengine;

TEST(GridPathfinder, StraightLinePath) {
    ai::GridPathfinder pf(10, 10);
    auto path = pf.find_path(0, 0, 5, 0);
    EXPECT_FALSE(path.empty());
    EXPECT_NEAR(path.back().x, 5.5f, 0.01f);
}

TEST(GridPathfinder, BlockedNoPath) {
    ai::GridPathfinder pf(5, 5);
    for (int x = 0; x < 5; ++x) pf.set_walkable(x, 2, false);
    auto path = pf.find_path(0, 0, 0, 4);
    EXPECT_TRUE(path.empty());
}

TEST(Steering, SeekProducesForce) {
    auto f = ai::Steering::seek({0, 0}, {0, 0}, {10, 0}, 5.0f);
    EXPECT_GT(f.x, 0.0f);
}

TEST(BehaviorTree, SelectorSucceedsOnFirst) {
    ai::SelectorNode sel("root");
    sel.add(std::make_unique<ai::ActionNode>("a", [](){ return ai::BTStatus::Success; }));
    sel.add(std::make_unique<ai::ActionNode>("b", [](){ return ai::BTStatus::Failure; }));
    ai::BehaviorTree bt(std::make_unique<ai::SelectorNode>(std::move(sel)));
    EXPECT_EQ(bt.tick(), ai::BTStatus::Success);
}

TEST(BehaviorTree, SequenceFailsOnFirst) {
    ai::SequenceNode seq("root");
    seq.add(std::make_unique<ai::ActionNode>("a", [](){ return ai::BTStatus::Failure; }));
    seq.add(std::make_unique<ai::ActionNode>("b", [](){ return ai::BTStatus::Success; }));
    ai::BehaviorTree bt(std::make_unique<ai::SequenceNode>(std::move(seq)));
    EXPECT_EQ(bt.tick(), ai::BTStatus::Failure);
}

TEST(ScriptVM, RunAndEmitSignal) {
    scene::Registry reg;
    script::ScriptVM vm(reg);
    script::Script s; s.name = "t"; s.local_count = 0;
    s.code.push_back({script::OpCode::PushInt, 5});
    s.code.push_back({script::OpCode::Print});
    s.code.push_back({script::OpCode::Return});
    vm.load(s);
    vm.start("t");
    vm.tick();
    bool fired = false;
    vm.signals().connect("hit", [&](){ fired = true; });
    vm.signals().emit("hit");
    EXPECT_TRUE(fired);
}

TEST(ReliableChannel, OrderedDelivery) {
    net::ReliableChannel rc;
    std::vector<net::Message> msgs;
    for (int i = 0; i < 3; ++i) {
        net::Message m; m.type = 1; m.payload = {static_cast<u8>(i)};
        rc.send(m);
    }
    // simulate receiving out of order
    auto& out = rc.outbox();
    std::vector<net::Message> reordered = {out[1], out[2], out[0]};
    auto delivered = rc.receive(reordered);
    EXPECT_EQ(delivered.size(), 3u);
    EXPECT_EQ(delivered[0].sequence, 1u);
    EXPECT_EQ(delivered[1].sequence, 2u);
    EXPECT_EQ(delivered[2].sequence, 3u);
}

TEST(SnapshotInterpolator, SampleBetween) {
    net::SnapshotInterpolator si;
    net::Snapshot a; a.timestamp = 0.0; a.entities.push_back({1, {0, 0}, 0});
    net::Snapshot b; b.timestamp = 1.0; b.entities.push_back({1, {10, 0}, 0});
    si.add_snapshot(a);
    si.add_snapshot(b);
    auto mid = si.sample(0.5);
    ASSERT_EQ(mid.entities.size(), 1u);
    EXPECT_NEAR(mid.entities[0].pos.x, 4.0f, 0.5f); // (0.5-0.1)/1.0 = 0.4 -> 4.0
}

TEST(SpatialAudio, AttenuationByDistance) {
    audio::Listener lis; lis.position = {0, 0};
    audio::SpatialVoice near_v; near_v.params.position = {10, 0}; near_v.base_gain = 1.0f;
    audio::SpatialVoice far_v; far_v.params.position = {500, 0}; far_v.base_gain = 1.0f;
    auto near_mix = audio::compute_spatial_mix(lis, near_v);
    auto far_mix = audio::compute_spatial_mix(lis, far_v);
    EXPECT_GT(near_mix.gain, far_mix.gain);
}

TEST(SkeletalAnimation, SampleClip) {
    anim::Skeleton skel;
    auto b0 = skel.add_bone("root");
    anim::AnimationClip clip; clip.duration = 1.0f;
    anim::BoneTrack track; track.bone_index = b0;
    track.keyframes.push_back({0.0f});
    track.keyframes.push_back({1.0f, {}, math::Quat::from_axis_angle({0,0,1}, 1.5f)});
    clip.tracks.push_back(track);
    auto poses = anim::sample_clip(clip, 0.5f);
    EXPECT_EQ(poses.size(), 1u);
}

TEST(AnimStateMachine, Transition) {
    anim::Skeleton skel; skel.add_bone("root");
    anim::AnimationClip clip; clip.duration = 1.0f;
    anim::AnimStateMachine asm_;
    anim::AnimState idle; idle.name = "idle"; idle.clip = &clip;
    idle.transitions["run"] = "run";
    anim::AnimState run; run.name = "run"; run.clip = &clip;
    asm_.add_state(idle);
    asm_.add_state(run);
    asm_.set_initial("idle");
    asm_.trigger("run");
    EXPECT_EQ(asm_.current(), "run");
}

TEST(EditorDashboard, AddPanelsAndLayout) {
    auto dash = editor::make_default_dashboard();
    EXPECT_GE(dash->panels().size(), 6u);
    dash->layout({1920, 1080});
    EXPECT_NE(dash->find("Inspector"), nullptr);
    dash->log("test");
    EXPECT_EQ(dash->console().back().text, "test");
}
