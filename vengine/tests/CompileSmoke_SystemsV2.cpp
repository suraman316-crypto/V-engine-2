// Compile-only smoke for v2 systems: scripting, AI, network, spatial audio,
// skeletal animation, editor dashboard.
#include <vengine/ai/BehaviorTree.hpp>
#include <vengine/ai/Navigation.hpp>
#include <vengine/animation/SkeletalAnimation.hpp>
#include <vengine/audio/SpatialAudio.hpp>
#include <vengine/editor/EditorDashboard.hpp>
#include <vengine/editor/Inspector.hpp>
#include <vengine/network/Network.hpp>
#include <vengine/scripting/ScriptVM.hpp>

int main() {
    using namespace vengine;

    // Editor dashboard
    auto dash = editor::make_default_dashboard();
    dash->show();
    dash->layout({1920, 1080});
    dash->log("hello");

    // Inspector + hierarchy
    scene::Registry reg;
    editor::Inspector insp(*dash);
    insp.select(reg.create("Player"), "Player");
    editor::Hierarchy hier(*dash);
    hier.set_scene(&reg);

    // Scripting VM
    script::ScriptVM vm(reg);
    script::Script s; s.name = "test"; s.local_count = 2;
    s.code.push_back({script::OpCode::PushInt, 5});
    s.code.push_back({script::OpCode::Print});
    s.code.push_back({script::OpCode::Return});
    vm.load(s);
    auto h = vm.start("test");
    (void)vm.tick();
    (void)vm.coroutine_count();
    vm.signals().connect("hit", [](){});
    vm.signals().emit("hit");

    // AI nav
    ai::GridPathfinder pf(32, 32);
    pf.set_walkable(0, 0, true);
    auto path = pf.find_path(0, 0, 5, 5);
    (void)path.size();
    ai::Steering::seek({0,0}, {0,0}, {10,10}, 5.0f);
    ai::Steering::arrive({0,0}, {0,0}, {10,10}, 5.0f, 50.0f);

    // Behavior tree
    ai::BehaviorTree bt(std::make_unique<ai::SelectorNode>("root"));
    auto seq = std::make_unique<ai::SequenceNode>("seq");
    seq->add(std::make_unique<ai::ConditionNode>([](){ return true; }));
    seq->add(std::make_unique<ai::ActionNode>("act", [](){ return ai::BTStatus::Success; }));
    (void)bt.tick();

    // Networking
    net::ReliableChannel rc;
    net::Message m; m.type = 1; m.payload = {1,2,3};
    rc.send(m);
    auto delivered = rc.receive({});
    (void)delivered.size();
    net::SnapshotInterpolator si;
    net::Snapshot snap; snap.timestamp = 1.0; snap.entities.push_back({1, {0,0}, 0});
    si.add_snapshot(snap);
    auto sampled = si.sample(1.0);
    (void)sampled.entities.size();
    std::vector<u8> buf;
    net::write_u32(buf, 12345);
    std::size_t off = 0;
    (void)net::read_u32(buf, off);

    // Spatial audio
    audio::Listener lis;
    audio::SpatialVoice v; v.playing = true;
    auto mix = audio::compute_spatial_mix(lis, v);
    (void)mix.gain;
    audio::StreamingBuffer sb; sb.loop = true;
    float l[8] = {0}, r[8] = {0};
    sb.push(l, r, 8);
    (void)sb.read(l, r, 4);
    audio::AudioBus bus; bus.name = "master";

    // Skeletal animation
    anim::Skeleton skel;
    auto b0 = skel.add_bone("root");
    auto b1 = skel.add_bone("spine", b0);
    (void)b1;
    anim::AnimationClip clip; clip.name = "idle"; clip.duration = 1.0f;
    anim::BoneTrack track; track.bone_index = b0;
    track.keyframes.push_back({0.0f});
    track.keyframes.push_back({1.0f, {}, math::Quat::from_axis_angle({0,0,1}, 1.5f)});
    clip.tracks.push_back(track);
    auto poses = anim::sample_clip(clip, 0.5f);
    (void)poses.size();
    auto skin = anim::compute_skinning(skel, poses);
    (void)skin.size();
    anim::BlendTree1D bt2;
    anim::BlendNode bn; bn.clip = &clip; bn.threshold = 0.0f;
    bt2.add(bn);
    (void)bt2.sample(0.5f, 0.5f);
    anim::AnimStateMachine asm_;
    anim::AnimState st; st.name = "idle"; st.clip = &clip;
    asm_.add_state(st);
    asm_.set_initial("idle");
    asm_.update(0.016f);
    asm_.trigger("run");

    return 0;
}
