// Compile-only smoke for the v2 asset/resource modules.
#include <vengine/assets/AssetManager.hpp>
#include <vengine/assets/Prefab.hpp>
#include <vengine/assets/TileMap.hpp>
#include <vengine/renderer/Font.hpp>
#include <vengine/renderer/Mesh.hpp>

#include <cstring>

int main() {
    using namespace vengine;

    // Asset manager
    assets::AssetManager am;
    auto id = am.import("player.png", assets::AssetType::Texture);
    am.acquire(id); am.release(id);
    am.find("player.png");
    am.check_hot_reload([](const std::string&){ return std::uint64_t(0); });

    // Mesh
    auto q = renderer::Mesh::quad(2, 2);
    (void)renderer::Mesh::nine_slice(100, 50, 8);
    (void)renderer::Mesh::circle(5, 24);
    (void)renderer::Mesh::polygon(5, 6);
    (void)renderer::Mesh::grid(100, 100, 8, 8);
    (void)q.indices.size();

    // Font
    renderer::Font f = renderer::make_default_font(1);
    auto sz = f.measure("Hello");
    (void)sz;
    auto placed = f.layout("Hi", {0,0}, 1.0f);
    (void)placed.size();

    // Tilemap
    assets::Tileset ts;
    assets::TileDef td; td.id = 1; td.collision = assets::TileCollision::Full;
    ts.add(td);
    assets::TileMap tm;
    assets::TileLayer l; l.width = 10; l.height = 10; l.solid = true;
    l.tiles.resize(100, 0);
    l.set(0, 0, 1);
    tm.add_layer(l);
    auto solids = tm.solid_boxes({{0,0},{100,100}}, ts);
    (void)solids.size();
    auto vis = tm.cull({{0,0},{100,100}}, ts);
    (void)vis.size();

    // Prefab
    assets::Prefab p;
    auto n = p.add_node("root");
    (void)n;
    scene::Registry reg;
    assets::instantiate(p, reg, [](scene::Entity, const std::string&, const std::vector<u8>&){});

    return 0;
}
