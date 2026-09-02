#include <vengine/assets/AssetDatabase.hpp>
#include <vengine/core/Logging.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace vengine::assets {

using serialization::JsonValue;
using serialization::parse_json;
using serialization::dump_json;
using serialization::atomic_write_text;

namespace {
std::string to_lower(std::string_view s) {
    std::string out{s};
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}
std::string_view ext_of(std::string_view path) {
    auto slash = path.find_last_of("/\\");
    auto start = (slash == std::string_view::npos) ? 0 : slash + 1;
    auto dot = path.find('.', start);
    if (dot == std::string_view::npos) return {};
    return path.substr(dot + 1);
}
} // namespace

const char* asset_type_name(AssetType t) noexcept {
    switch (t) {
        case AssetType::Unknown:        return "Unknown";
        case AssetType::Texture:        return "Texture";
        case AssetType::TextureAtlas:   return "TextureAtlas";
        case AssetType::SpriteSheet:    return "SpriteSheet";
        case AssetType::Font:           return "Font";
        case AssetType::Audio:          return "Audio";
        case AssetType::Scene:          return "Scene";
        case AssetType::Prefab:         return "Prefab";
        case AssetType::Material:       return "Material";
        case AssetType::Shader:         return "Shader";
        case AssetType::Animation:      return "Animation";
        case AssetType::Tilemap:        return "Tilemap";
        case AssetType::ParticleConfig: return "ParticleConfig";
        case AssetType::Json:           return "Json";
    }
    return "Unknown";
}

AssetType guess_type_from_extension(std::string_view path) noexcept {
    auto e = to_lower(ext_of(path));
    if (e == "png" || e == "jpg" || e == "jpeg" || e == "webp") return AssetType::Texture;
    if (e == "atlas")   return AssetType::TextureAtlas;
    if (e == "spritesheet") return AssetType::SpriteSheet;
    if (e == "ttf" || e == "otf") return AssetType::Font;
    if (e == "wav" || e == "ogg" || e == "mp3" || e == "flac") return AssetType::Audio;
    if (e == "vscene")   return AssetType::Scene;
    if (e == "prefab")   return AssetType::Prefab;
    if (e == "mat")      return AssetType::Material;
    if (e == "glsl" || e == "vert" || e == "frag") return AssetType::Shader;
    if (e == "anim")     return AssetType::Animation;
    if (e == "tilemap")  return AssetType::Tilemap;
    if (e == "particles") return AssetType::ParticleConfig;
    if (e == "json")     return AssetType::Json;
    return AssetType::Unknown;
}

AssetId AssetDatabase::next_id() noexcept {
    const std::lock_guard lock(mutex_);
    return next_id_++;
}

void AssetDatabase::upsert(AssetRecord rec) {
    const std::lock_guard lock(mutex_);
    by_id_[rec.id] = std::move(rec);
}

bool AssetDatabase::remove(AssetId id) {
    const std::lock_guard lock(mutex_);
    return by_id_.erase(id) > 0;
}

const AssetRecord* AssetDatabase::find(AssetId id) const {
    const std::lock_guard lock(mutex_);
    auto it = by_id_.find(id);
    return it == by_id_.end() ? nullptr : &it->second;
}

const AssetRecord* AssetDatabase::find_by_source(std::string_view source) const {
    const std::lock_guard lock(mutex_);
    for (const auto& [_, rec] : by_id_) {
        if (rec.source_path == source) return &rec;
    }
    return nullptr;
}

const AssetRecord* AssetDatabase::find_by_name(std::string_view name) const {
    const std::lock_guard lock(mutex_);
    for (const auto& [_, rec] : by_id_) {
        if (rec.name == name) return &rec;
    }
    return nullptr;
}

std::size_t AssetDatabase::size() const noexcept {
    const std::lock_guard lock(mutex_);
    return by_id_.size();
}

std::vector<AssetRecord> AssetDatabase::all() const {
    const std::lock_guard lock(mutex_);
    std::vector<AssetRecord> out;
    out.reserve(by_id_.size());
    for (const auto& [_, rec] : by_id_) out.push_back(rec);
    return out;
}

Result<void> AssetDatabase::save(std::string_view path) const {
    JsonValue root = JsonValue::make_object();
    root.set("type", std::string{"vengine.assetdb"});
    root.set("version", 1);
    JsonValue arr = JsonValue::make_array();
    {
        const std::lock_guard lock(mutex_);
        for (const auto& [_, rec] : by_id_) {
            JsonValue j = JsonValue::make_object();
            j.set("id", static_cast<i64>(rec.id));
            j.set("type", std::string{asset_type_name(rec.type)});
            j.set("source", rec.source_path);
            j.set("imported", rec.imported_path);
            j.set("name", rec.name);
            j.set("modified", rec.modified_timestamp);
            j.set("version", static_cast<i64>(rec.version));
            JsonValue deps = JsonValue::make_array();
            for (AssetId d : rec.dependencies) deps.array().push_back(static_cast<i64>(d));
            j.set("dependencies", deps);
            arr.array().push_back(std::move(j));
        }
    }
    root.set("assets", arr);
    return serialization::atomic_write_text(path, serialization::dump_json(root));
}

Result<void> AssetDatabase::load(std::string_view path) {
    std::ifstream in{std::string(path)};
    if (!in) return Result<void>::Ok(); // missing db -> start empty
    std::ostringstream ss;
    ss << in.rdbuf();
    auto parsed = serialization::parse_json(ss.str());
    if (!parsed.ok()) return parsed.error();
    const auto& root = parsed.value();
    const auto* arr = root.find("assets");
    if (!arr || !arr->is_array()) {
        return Error::from(ErrorCode::DeserializationFailed, "assetdb: missing 'assets'");
    }

    AssetDatabase fresh;
    AssetId max_id = 1;
    for (const auto& a : arr->array()) {
        AssetRecord rec;
        rec.id = static_cast<AssetId>(a.find("id") ? a.find("id")->as_int() : 0);
        if (rec.id == 0) continue;
        max_id = std::max(max_id, rec.id + 1);
        if (const auto* t = a.find("type")) {
            std::string ts = t->as_string();
            // map name->type
            for (int i = 0; i <= static_cast<int>(AssetType::Json); ++i) {
                auto at = static_cast<AssetType>(i);
                if (asset_type_name(at) == ts) { rec.type = at; break; }
            }
        }
        if (const auto* s = a.find("source"))   rec.source_path = s->as_string();
        if (const auto* i2 = a.find("imported")) rec.imported_path = i2->as_string();
        if (const auto* n = a.find("name"))     rec.name = n->as_string();
        if (const auto* m = a.find("modified")) rec.modified_timestamp = m->as_int();
        if (const auto* v = a.find("version"))  rec.version = static_cast<std::uint32_t>(v->as_int(1));
        if (const auto* d = a.find("dependencies"); d && d->is_array()) {
            for (const auto& dv : d->array()) rec.dependencies.push_back(static_cast<AssetId>(dv.as_int()));
        }
        fresh.upsert(std::move(rec));
    }
    fresh.next_id_ = max_id;

    // Adopt loaded state under our lock (AssetDatabase holds a non-movable
    // mutex, so the whole object is not move-assignable).
    auto fresh_records = fresh.all();
    AssetId fresh_next = fresh.next_id_;
    {
        const std::lock_guard lock(mutex_);
        by_id_.clear();
        for (auto& r : fresh_records) by_id_.emplace(r.id, std::move(r));
        next_id_ = fresh_next;
    }
    return Result<void>::Ok();
}

std::vector<AssetId> AssetDatabase::find_missing(
    const std::function<bool(std::string_view)>& exists_fn) const {
    std::vector<AssetId> missing;
    const std::lock_guard lock(mutex_);
    for (const auto& [_, rec] : by_id_) {
        if (!rec.source_path.empty() && !exists_fn(rec.source_path)) {
            missing.push_back(rec.id);
        }
    }
    return missing;
}

} // namespace vengine::assets
