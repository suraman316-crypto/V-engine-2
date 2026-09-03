#include <vengine/serialization/SceneSerializer.hpp>
#include <vengine/core/Logging.hpp>

#include <fstream>
#include <optional>
#include <sstream>

namespace vengine::serialization {

namespace {

JsonValue transform_to_json(const math::Transform2D& t) {
    JsonValue v = JsonValue::make_object();
    auto pos = JsonValue::make_object();
    pos.set("x", t.position.x);
    pos.set("y", t.position.y);
    v.set("position", pos);
    v.set("rotation", t.rotation);
    auto scl = JsonValue::make_object();
    scl.set("x", t.scale.x);
    scl.set("y", t.scale.y);
    v.set("scale", scl);
    return v;
}

bool transform_from_json(const JsonValue* j, math::Transform2D& out) {
    if (!j || !j->is_object()) return false;
    if (const auto* p = j->find("position")) {
        out.position.x = static_cast<float>(p->find("x") ? p->find("x")->as_float() : 0.0);
        out.position.y = static_cast<float>(p->find("y") ? p->find("y")->as_float() : 0.0);
    }
    if (const auto* r = j->find("rotation")) out.rotation = static_cast<float>(r->as_float());
    if (const auto* sc = j->find("scale")) {
        out.scale.x = static_cast<float>(sc->find("x") ? sc->find("x")->as_float(1.0) : 1.0);
        out.scale.y = static_cast<float>(sc->find("y") ? sc->find("y")->as_float(1.0) : 1.0);
    }
    return true;
}

const char* body_type_name(components::BodyType t) {
    switch (t) {
        case components::BodyType::Static:    return "Static";
        case components::BodyType::Dynamic:   return "Dynamic";
        case components::BodyType::Kinematic: return "Kinematic";
    }
    return "Dynamic";
}

std::optional<components::BodyType> body_type_from_name(std::string_view s) {
    if (s == "Static")    return components::BodyType::Static;
    if (s == "Dynamic")   return components::BodyType::Dynamic;
    if (s == "Kinematic") return components::BodyType::Kinematic;
    return std::nullopt;
}

} // namespace

std::string serialize_scene(const scene::Scene& scene) {
    JsonValue root = JsonValue::make_object();
    root.set("type", std::string{"vengine.scene"});
    root.set("version", 1);
    root.set("name", scene.name());

    JsonValue entities = JsonValue::make_array();
    const auto& reg = scene.registry();
    // Entities are serialized by walking the TransformComponent storage; the
    // editor always adds a transform, so every persisted entity is captured.
    reg.view<components::TransformComponent>([&](scene::Entity e,
                                                 const components::TransformComponent& tc) {
        JsonValue ent = JsonValue::make_object();
        ent.set("id", static_cast<i64>(e.raw));
        ent.set("name", reg.name(e));
        ent.set("transform", transform_to_json(tc.value));

        JsonValue comps = JsonValue::make_array();
        if (auto* spr = reg.get<components::SpriteRenderer>(e)) {
            JsonValue c = JsonValue::make_object();
            c.set("type", std::string{"SpriteRenderer"});
            c.set("texture", spr->texture_asset);
            auto uv = JsonValue::make_object();
            uv.set("x", spr->uv.x); uv.set("y", spr->uv.y);
            uv.set("w", spr->uv.w); uv.set("h", spr->uv.h);
            c.set("uv", uv);
            c.set("flipX", spr->flip_x);
            c.set("flipY", spr->flip_y);
            c.set("layer", spr->layer);
            c.set("order", spr->order_in_layer);
            comps.array().push_back(std::move(c));
        }
        if (auto* rb = reg.get<components::Rigidbody2D>(e)) {
            JsonValue c = JsonValue::make_object();
            c.set("type", std::string{"Rigidbody2D"});
            c.set("bodyType", std::string{body_type_name(rb->type)});
            c.set("fixedRotation", rb->fixed_rotation);
            c.set("gravityScale", rb->gravity_scale);
            c.set("collisionLayer", static_cast<i64>(rb->collision_layer));
            c.set("collisionMask", static_cast<i64>(rb->collision_mask));
            c.set("isSensor", rb->is_sensor);
            comps.array().push_back(std::move(c));
        }
        if (auto* bc = reg.get<components::BoxCollider>(e)) {
            JsonValue c = JsonValue::make_object();
            c.set("type", std::string{"BoxCollider"});
            auto sz = JsonValue::make_object();
            sz.set("x", bc->size.x); sz.set("y", bc->size.y);
            c.set("size", sz);
            c.set("isTrigger", bc->is_trigger);
            comps.array().push_back(std::move(c));
        }
        if (auto* cam = reg.get<components::Camera>(e)) {
            JsonValue c = JsonValue::make_object();
            c.set("type", std::string{"Camera"});
            c.set("zoom", cam->zoom);
            c.set("active", cam->active);
            comps.array().push_back(std::move(c));
        }
        ent.set("components", comps);
        entities.array().push_back(std::move(ent));
    });

    root.set("entities", entities);
    return dump_json(root);
}

Result<void> deserialize_scene(std::string_view text, scene::Scene& out) {
    auto parsed = parse_json(text);
    if (!parsed.ok()) return parsed.error();
    const auto& root = parsed.value();
    if (!root.is_object()) {
        return Error::from(ErrorCode::InvalidScene, "scene root is not an object");
    }
    const auto* type = root.find("type");
    if (!type || type->as_string() != "vengine.scene") {
        return Error::from(ErrorCode::InvalidScene, "missing/invalid 'type' field");
    }
    const auto* ents = root.find("entities");
    if (!ents || !ents->is_array()) {
        return Error::from(ErrorCode::InvalidScene, "'entities' must be an array");
    }

    out.destroy();
    for (const auto& ent_json : ents->array()) {
        if (!ent_json.is_object()) {
            return Error::from(ErrorCode::InvalidScene, "entity is not an object");
        }
        std::string name = ent_json.find("name") ? ent_json.find("name")->as_string() : std::string{};
        scene::Entity e = out.create_entity(name);

        if (const auto* tj = ent_json.find("transform")) {
            components::TransformComponent tc;
            if (!transform_from_json(tj, tc.value)) {
                return Error::from(ErrorCode::InvalidScene, "malformed transform");
            }
            out.registry().add<components::TransformComponent>(e, tc);
        }

        if (const auto* comps = ent_json.find("components"); comps && comps->is_array()) {
            for (const auto& c : comps->array()) {
                if (!c.is_object()) continue;
                std::string ctype = c.find("type") ? c.find("type")->as_string() : std::string{};
                if (ctype == "SpriteRenderer") {
                    components::SpriteRenderer spr;
                    if (const auto* t = c.find("texture")) spr.texture_asset = t->as_string();
                    if (const auto* uv = c.find("uv")) {
                        spr.uv.x = static_cast<float>(uv->find("x") ? uv->find("x")->as_float() : 0.0);
                        spr.uv.y = static_cast<float>(uv->find("y") ? uv->find("y")->as_float() : 0.0);
                        spr.uv.w = static_cast<float>(uv->find("w") ? uv->find("w")->as_float(1.0) : 1.0);
                        spr.uv.h = static_cast<float>(uv->find("h") ? uv->find("h")->as_float(1.0) : 1.0);
                    }
                    if (const auto* fx = c.find("flipX")) spr.flip_x = fx->as_bool();
                    if (const auto* fy = c.find("flipY")) spr.flip_y = fy->as_bool();
                    if (const auto* l = c.find("layer"))  spr.layer = static_cast<int>(l->as_int());
                    if (const auto* o = c.find("order"))  spr.order_in_layer = static_cast<int>(o->as_int());
                    out.registry().add<components::SpriteRenderer>(e, spr);
                } else if (ctype == "Rigidbody2D") {
                    components::Rigidbody2D rb;
                    if (const auto* bt = c.find("bodyType")) {
                        if (auto t = body_type_from_name(bt->as_string())) rb.type = *t;
                        else return Error::from(ErrorCode::InvalidComponent, "bad bodyType");
                    }
                    if (const auto* fr = c.find("fixedRotation")) rb.fixed_rotation = fr->as_bool();
                    if (const auto* gs = c.find("gravityScale"))  rb.gravity_scale = static_cast<float>(gs->as_float(1.0));
                    if (const auto* cl = c.find("collisionLayer")) rb.collision_layer = static_cast<std::uint16_t>(cl->as_int(0xFFFF));
                    if (const auto* cm = c.find("collisionMask"))  rb.collision_mask  = static_cast<std::uint16_t>(cm->as_int(0xFFFF));
                    if (const auto* is = c.find("isSensor")) rb.is_sensor = is->as_bool();
                    out.registry().add<components::Rigidbody2D>(e, rb);
                } else if (ctype == "BoxCollider") {
                    components::BoxCollider bc;
                    if (const auto* sz = c.find("size")) {
                        bc.size.x = static_cast<float>(sz->find("x") ? sz->find("x")->as_float(1.0) : 1.0);
                        bc.size.y = static_cast<float>(sz->find("y") ? sz->find("y")->as_float(1.0) : 1.0);
                    }
                    if (const auto* it = c.find("isTrigger")) bc.is_trigger = it->as_bool();
                    out.registry().add<components::BoxCollider>(e, bc);
                } else if (ctype == "Camera") {
                    components::Camera cam;
                    if (const auto* z = c.find("zoom")) cam.zoom = static_cast<float>(z->as_float(1.0));
                    if (const auto* a = c.find("active")) cam.active = a->as_bool(true);
                    out.registry().add<components::Camera>(e, cam);
                }
                // Unknown component types are ignored rather than fatal, so a
                // newer-format scene still loads on an older engine (backward
                // compatibility, README rule #27).
            }
        }
    }
    return Result<void>::Ok();
}

Result<void> save_scene(std::string_view path, const scene::Scene& scene) {
    std::string text = serialize_scene(scene);
    auto res = atomic_write_text(path, text);
    if (!res.ok()) VENGINE_LOG_ERROR("Scene", "save_scene failed: %s", res.error().format().c_str());
    return res;
}

Result<void> load_scene(std::string_view path, scene::Scene& out) {
    std::ifstream in{std::string(path)};
    if (!in) {
        return Error::from(ErrorCode::FileNotFound,
            "scene file not found: " + std::string(path));
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return deserialize_scene(ss.str(), out);
}

} // namespace vengine::serialization
