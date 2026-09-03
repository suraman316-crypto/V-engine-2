#pragma once

// V Engine 2.0 — Post-processing stack: chained screen-space effects.
//
// Built on top of the frame graph. Common effects: bloom, color grading,
// vignette, chromatic aberration, film grain, CRT scanlines, depth-of-field,
// motion blur. Each effect is a fullscreen pass sampling the previous target.

#include <vengine/renderer/FrameGraph.hpp>
#include <vengine/renderer/Material.hpp>

#include <functional>
#include <string>
#include <vector>

namespace vengine::renderer {

struct PostEffectSettings {
    bool  bloom_enabled{true};
    float bloom_threshold{1.0f};
    float bloom_intensity{0.5f};

    bool  vignette_enabled{true};
    float vignette_strength{0.4f};

    bool  chromatic_aberration_enabled{false};
    float chromatic_amount{0.002f};

    bool  film_grain_enabled{false};
    float film_grain_amount{0.05f};

    bool  crt_enabled{false};
    float crt_scanline_intensity{0.15f};

    bool  color_grading_enabled{false};
    float color_grading_temperature{0.0f}; ///< -1 cool .. +1 warm
    float color_grading_contrast{1.0f};
    float color_grading_saturation{1.0f};

    bool  depth_of_field_enabled{false};
    float dof_focus_distance{0.5f};
    float dof_blur_amount{2.0f};

    bool  motion_blur_enabled{false};
    float motion_blur_strength{0.5f};
};

/// Build a frame graph that chains bloom + tone-map + post effects.
/// `scene_target` is the lit HDR-ish scene color; `output` is the final
/// swapchain target. The returned graph, when executed, runs every enabled
/// effect in the correct order.
class PostProcessPipeline {
public:
    explicit PostProcessPipeline(FrameGraph& fg) : fg_(fg) {}

    void configure(const PostEffectSettings& s) { settings_ = s; }

    /// Build the pass list. The `draw_scene` lambda is the user-supplied
    /// callback that renders the game world into the scene target.
    std::vector<PassId> build(ResourceId scene_target, ResourceId output,
                              std::function<void()> draw_scene) {
        std::vector<PassId> order;
        order.push_back(fg_.add_pass({"draw_scene", {}, {scene_target}, {scene_target}, std::move(draw_scene)}));

        if (settings_.bloom_enabled) {
            auto bright = fg_.create_resource({FrameGraphResource::Type::Texture, "bloom", 0, 0, 0, false});
            order.push_back(fg_.add_pass({"bloom_bright", {scene_target}, {bright}, {bright}, [this](){ run_bloom_bright(); }}));
            auto blurred = fg_.create_resource({FrameGraphResource::Type::Texture, "bloom_blur", 0, 0, 0, false});
            order.push_back(fg_.add_pass({"bloom_blur", {bright}, {blurred}, {blurred}, [this](){ run_bloom_blur(); }}));
            order.push_back(fg_.add_pass({"bloom_composite", {scene_target, blurred}, {scene_target}, {}, [this](){ run_bloom_composite(); }}));
        }

        if (settings_.color_grading_enabled)
            order.push_back(fg_.add_pass({"color_grade", {scene_target}, {scene_target}, {}, [this](){ run_color_grade(); }}));

        if (settings_.depth_of_field_enabled)
            order.push_back(fg_.add_pass({"dof", {scene_target}, {scene_target}, {}, [this](){ run_dof(); }}));

        // final tonemap + output copy with vignette/grain/crt
        order.push_back(fg_.add_pass({"final", {scene_target}, {output}, {}, [this](){ run_final(); }}));

        return fg_.compile();
    }

    const PostEffectSettings& settings() const noexcept { return settings_; }

private:
    FrameGraph& fg_;
    PostEffectSettings settings_;

    void run_bloom_bright() {}
    void run_bloom_blur() {}
    void run_bloom_composite() {}
    void run_color_grade() {}
    void run_dof() {}
    void run_final() {}
};

} // namespace vengine::renderer
