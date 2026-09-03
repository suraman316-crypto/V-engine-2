#pragma once

#include <vengine/Common.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/serialization/Serialization.hpp>

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vengine::assets {

enum class AssetType : std::uint8_t {
    Unknown = 0,
    Texture,
    TextureAtlas,
    SpriteSheet,
    Font,
    Audio,
    Scene,
    Prefab,
    Material,
    Shader,
    Animation,
    Tilemap,
    ParticleConfig,
    Json,
};

const char* asset_type_name(AssetType t) noexcept;
AssetType   guess_type_from_extension(std::string_view path) noexcept;

using AssetId = std::uint64_t;

struct AssetRecord {
    AssetId      id{0};
    AssetType    type{AssetType::Unknown};
    std::string  source_path;
    std::string  imported_path;
    std::string  name;
    std::int64_t modified_timestamp{0};
    std::uint32_t version{1};
    std::vector<AssetId> dependencies;
};

/// The asset database tracks every imported asset. Persistence is atomic.
/// All mutating operations are thread-safe.
class AssetDatabase {
public:
    AssetDatabase() = default;

    AssetId next_id() noexcept;

    void   upsert(AssetRecord rec);
    bool   remove(AssetId id);
    const AssetRecord* find(AssetId id) const;
    const AssetRecord* find_by_source(std::string_view source) const;
    const AssetRecord* find_by_name(std::string_view name) const;

    std::size_t size() const noexcept;
    std::vector<AssetRecord> all() const;

    Result<void> save(std::string_view path) const;
    Result<void> load(std::string_view path);

    /// Detect missing source files. `exists_fn` is provided by the platform
    /// layer; returns ids whose source_path no longer resolves.
    std::vector<AssetId> find_missing(
        const std::function<bool(std::string_view)>& exists_fn) const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<AssetId, AssetRecord> by_id_;
    AssetId next_id_{1};
};

} // namespace vengine::assets
