// Compile-only smoke test for the v2 renderer modules.
#include <vengine/renderer/FrameGraph.hpp>
#include <vengine/renderer/Lighting.hpp>
#include <vengine/renderer/Material.hpp>
#include <vengine/renderer/PostProcess.hpp>
#include <vengine/renderer/ShaderLibrary.hpp>
#include <vengine/renderer/TextureAtlas.hpp>

#include <vengine/math/Geometry.hpp>
#include <vengine/math/Quat.hpp>
#include <vengine/math/Spline.hpp>
#include <vengine/math/Transform.hpp>

int main() {
    using namespace vengine;

    // Shader library
    auto sprite = renderer::ShaderLibrary::sprite();
    (void)sprite;
    auto lit = renderer::ShaderLibrary::lit_sprite();
    (void)lit;
    renderer::ShaderLibrary::post_crt();
    renderer::ShaderLibrary::blur();
    renderer::ShaderLibrary::particle();
    renderer::ShaderLibrary::text_sdf();
    renderer::ShaderLibrary::shape();

    // Material
    renderer::Material m;
    m.set_float("u_time", 1.0f);
    m.set_color("u_tint", math::Color::red());
    m.bind_texture("u_tex", 5);
    m.blend = renderer::BlendMode::Additive;
    (void)m.sort_key();
    renderer::MaterialPool pool;
    auto mh = pool.create(m);
    pool.get(mh);
    pool.destroy(mh);

    // Frame graph
    renderer::FrameGraph fg;
    auto r1 = fg.create_resource({renderer::FrameGraphResource::Type::Texture, "scene", 1920, 1080, 0, false});
    auto r2 = fg.create_resource({renderer::FrameGraphResource::Type::Texture, "out", 1920, 1080, 0, true});
    (void)fg.add_pass({"draw", {}, {r1}, {r1}, [](){}});
    (void)fg.add_pass({"post", {r1}, {r2}, {}, [](){}});
    auto order = fg.compile();
    fg.execute(order);

    // Lighting
    renderer::LightSystem ls;
    ls.clear();
    ls.add_point({{100,100}, 256.0f, 1.0f, math::Color::white()});
    ls.directional().direction = {0, -1};
    ls.ambient().intensity = 0.4f;
    auto c = ls.sample({100, 100});
    (void)c;
    std::vector<math::AABB> occ{{ {-10,-10},{10,10} }};
    (void)ls.shadow_factor({0,0}, occ);

    // Texture atlas
    renderer::TextureAtlas atlas(512, 512);
    auto pr = atlas.add(64, 64, 1);
    (void)pr;
    (void)atlas.count();

    // Post process
    renderer::PostEffectSettings s;
    s.bloom_enabled = true;
    renderer::PostProcessPipeline pp(fg);
    pp.configure(s);
    auto order2 = pp.build(r1, r2, [](){});
    (void)order2;

    return 0;
}
