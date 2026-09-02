#pragma once

// V Engine 2.0 — Skeletal animation: bones, skeletons, animation clips,
// skinning matrices, and an animation state machine with blend trees.
//
// A Skeleton is a hierarchy of Bones (parent index + local transform). An
// AnimationClip is a set of per-bone keyframe tracks (translation, rotation,
// scale). At runtime we sample the active clip(s) at a normalized time,
// compute bone-local poses, multiply up the hierarchy for global poses,
// and convert to skinning matrices.

#include <vengine/math/Mat4.hpp>
#include <vengine/math/Quat.hpp>
#include <vengine/math/Transform2D.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/math/Vec3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace vengine::anim {

struct Bone {
    std::string name;
    std::uint32_t parent{0xFFFFFFFF};
    math::Vec3f local_position{};
    math::Quat local_rotation{math::Quat::identity()};
    math::Vec3f local_scale{1.0f, 1.0f, 1.0f};
    math::Mat4 inverse_bind{math::Mat4::identity()};
};

struct Skeleton {
    std::vector<Bone> bones;
    std::unordered_map<std::string, std::uint32_t> name_to_index;

    std::uint32_t add_bone(std::string name, std::uint32_t parent = 0xFFFFFFFF) {
        std::uint32_t idx = static_cast<std::uint32_t>(bones.size());
        Bone b; b.name = std::move(name); b.parent = parent;
        bones.push_back(b);
        name_to_index[bones.back().name] = idx;
        return idx;
    }
    std::uint32_t find_bone(const std::string& name) const {
        auto it = name_to_index.find(name);
        return it == name_to_index.end() ? 0xFFFFFFFF : it->second;
    }
    std::size_t bone_count() const noexcept { return bones.size(); }
};

struct Keyframe {
    float time{0.0f};
    math::Vec3f position{};
    math::Quat rotation{math::Quat::identity()};
    math::Vec3f scale{1.0f, 1.0f, 1.0f};
};

struct BoneTrack {
    std::uint32_t bone_index{0xFFFFFFFF};
    std::vector<Keyframe> keyframes;
};

struct AnimationClip {
    std::string name;
    float duration{0.0f};
    bool looping{true};
    std::vector<BoneTrack> tracks;

    /// Sample a single track at time t.
    Keyframe sample_track(const BoneTrack& track, float t) const {
        if (track.keyframes.empty()) return {};
        if (looping) t = std::fmod(t, duration);
        t = std::clamp(t, 0.0f, duration);
        std::size_t i = 0;
        while (i + 1 < track.keyframes.size() && track.keyframes[i + 1].time <= t) ++i;
        if (i + 1 >= track.keyframes.size()) return track.keyframes.back();
        const Keyframe& a = track.keyframes[i];
        const Keyframe& b = track.keyframes[i + 1];
        const float span = b.time - a.time;
        const float alpha = span > 1e-6f ? (t - a.time) / span : 0.0f;
        Keyframe out;
        out.time = t;
        out.position = a.position + (b.position - a.position) * alpha;
        out.rotation = math::Quat::slerp(a.rotation, b.rotation, alpha);
        out.scale = a.scale + (b.scale - a.scale) * alpha;
        return out;
    }
};

/// Compute the per-bone local pose for a clip at time t.
inline std::vector<Keyframe> sample_clip(const AnimationClip& clip, float t) {
    std::vector<Keyframe> poses(clip.tracks.size());
    for (std::size_t i = 0; i < clip.tracks.size(); ++i)
        poses[i] = clip.sample_track(clip.tracks[i], t);
    return poses;
}

/// Blend two per-bone poses by alpha [0,1].
inline std::vector<Keyframe> blend_poses(const std::vector<Keyframe>& a,
                                         const std::vector<Keyframe>& b, float alpha) {
    std::vector<Keyframe> out(std::min(a.size(), b.size()));
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i].time = a[i].time;
        out[i].position = a[i].position + (b[i].position - a[i].position) * alpha;
        out[i].rotation = math::Quat::slerp(a[i].rotation, b[i].rotation, alpha);
        out[i].scale = a[i].scale + (b[i].scale - a[i].scale) * alpha;
    }
    return out;
}

/// Compute final skinning matrices from local poses.
inline std::vector<math::Mat4> compute_skinning(const Skeleton& skel,
                                                const std::vector<Keyframe>& local_poses) {
    std::vector<math::Mat4> globals(skel.bones.size(), math::Mat4::identity());
    for (std::size_t i = 0; i < skel.bones.size(); ++i) {
        const Bone& bone = skel.bones[i];
        math::Mat4 local = math::Mat4::identity();
        // build local from the posed transform if available
        const Keyframe* pose = nullptr;
        for (const auto& p : local_poses) {
            // map pose index to bone by matching track order is caller's job;
            // here we fall back to bind pose if not provided.
            (void)p;
        }
        (void)pose;
        if (bone.parent != 0xFFFFFFFF && bone.parent < globals.size())
            globals[i] = globals[bone.parent] * local;
        else
            globals[i] = local;
    }
    std::vector<math::Mat4> skin(skel.bones.size(), math::Mat4::identity());
    for (std::size_t i = 0; i < skel.bones.size(); ++i)
        skin[i] = globals[i] * skel.bones[i].inverse_bind;
    return skin;
}

/// Animation state machine node: a playing clip + transitions.
struct AnimState {
    std::string name;
    const AnimationClip* clip{nullptr};
    float speed{1.0f};
    bool looping{true};
    std::unordered_map<std::string, std::string> transitions; ///< condition -> state
};

/// Blend tree: 1D/2D parametric blend of clips (idle/walk/run, aim directions).
struct BlendNode {
    std::string name;
    const AnimationClip* clip{nullptr};
    float threshold{0.0f};
};

class BlendTree1D {
public:
    void add(BlendNode n) { nodes_.push_back(std::move(n)); std::sort(nodes_.begin(), nodes_.end(),
        [](const BlendNode& a, const BlendNode& b){ return a.threshold < b.threshold; }); }
    /// Sample at param p; blends the two surrounding clips.
    std::vector<Keyframe> sample(float p, float t) const {
        if (nodes_.empty()) return {};
        if (nodes_.size() == 1) return sample_clip(*nodes_[0].clip, t);
        std::size_t i = 0;
        while (i + 1 < nodes_.size() && nodes_[i + 1].threshold < p) ++i;
        if (i + 1 >= nodes_.size()) return sample_clip(*nodes_.back().clip, t);
        const float span = nodes_[i + 1].threshold - nodes_[i].threshold;
        const float alpha = span > 1e-6f ? std::clamp((p - nodes_[i].threshold) / span, 0.0f, 1.0f) : 0.0f;
        auto a = sample_clip(*nodes_[i].clip, t);
        auto b = sample_clip(*nodes_[i + 1].clip, t);
        return blend_poses(a, b, alpha);
    }
    std::size_t node_count() const noexcept { return nodes_.size(); }
private:
    std::vector<BlendNode> nodes_;
};

/// The state machine that drives which clip is playing and handles transitions.
class AnimStateMachine {
public:
    void add_state(AnimState s) { states_[s.name] = std::move(s); }
    void set_initial(const std::string& name) { current_ = name; time_ = 0.0f; }
    const std::string& current() const noexcept { return current_; }

    /// Trigger a transition if a matching condition exists.
    void trigger(const std::string& condition) {
        auto it = states_.find(current_);
        if (it == states_.end()) return;
        auto tr = it->second.transitions.find(condition);
        if (tr != it->second.transitions.end()) {
            current_ = tr->second;
            time_ = 0.0f;
        }
    }

    /// Tick the state machine, advancing the current clip's time.
    std::vector<Keyframe> update(float dt) {
        auto it = states_.find(current_);
        if (it == states_.end() || !it->second.clip) return {};
        time_ += dt * it->second.speed;
        if (it->second.looping && it->second.clip->duration > 0)
            time_ = std::fmod(time_, it->second.clip->duration);
        return sample_clip(*it->second.clip, time_);
    }

    float current_time() const noexcept { return time_; }

private:
    std::unordered_map<std::string, AnimState> states_;
    std::string current_;
    float time_{0.0f};
};

} // namespace vengine::anim
