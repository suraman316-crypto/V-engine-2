#pragma once

// V Engine 2.0 — Asset system: load, cache, and reference-count engine
// resources (textures, meshes, fonts, audio clips, prefabs, scripts).
//
// Async-loadable: assets register a "load" job that decodes bytes from disk
// (or an Android asset stream), then a "finalize" step that uploads to GPU.
// Hot-reload watches file mtimes and re-loads. AssetHandle is a typed,
// type-safe id.

#include <vengine/Common.hpp>
#include <vengine/core/Types.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vengine::assets {

enum class AssetType : u8 {
    Texture,
    Mesh,
    Font,
    AudioClip,
    Prefab,
    Script,
    Shader,
    AnimationClip,
    TileMap,
    Skeleton,
    Count,
};

struct AssetMeta {
    std::string path;
    AssetType type{AssetType::Texture};
    std::uint64_t mtime{0};
    std::uint32_t refcount{0};
    bool loaded{false};
    bool loading{false};
    std::string tag; ///< logical group for streaming
};

using AssetId = std::uint32_t;
constexpr AssetId kInvalidAsset = 0;

struct LoadRequest {
    AssetId id{kInvalidAsset};
    AssetType type{AssetType::Texture};
    std::string path;
    std::function<bool(const std::vector<u8>& bytes)> on_bytes; ///< decode on a worker
    std::function<void()> on_finalize;                          ///< upload on main
    bool done{false};
};

class AssetManager {
public:
    AssetId import(std::string path, AssetType type) {
        AssetId id = static_cast<AssetId>(metas_.size() + 1);
        AssetMeta m; m.path = std::move(path); m.type = type;
        metas_.push_back(std::move(m));
        path_to_id_[metas_.back().path] = id;
        return id;
    }

    AssetId find(const std::string& path) const {
        auto it = path_to_id_.find(path);
        return it == path_to_id_.end() ? kInvalidAsset : it->second;
    }

    void acquire(AssetId id) {
        if (id == kInvalidAsset || id > metas_.size()) return;
        ++metas_[id - 1].refcount;
    }

    void release(AssetId id) {
        if (id == kInvalidAsset || id > metas_.size()) return;
        auto& m = metas_[id - 1];
        if (m.refcount > 0) --m.refcount;
        if (m.refcount == 0 && m.loaded) {
            m.loaded = false; // mark for unload
            // (backend owns GPU free; AssetManager just drops the ref)
        }
    }

    AssetMeta* meta(AssetId id) {
        if (id == kInvalidAsset || id > metas_.size()) return nullptr;
        return &metas_[id - 1];
    }

    /// Check loaded asset mtimes and queue hot-reloads.
    void check_hot_reload(const std::function<std::uint64_t(const std::string&)>& get_mtime) {
        for (auto& m : metas_) {
            if (m.path.empty()) continue;
            const std::uint64_t cur = get_mtime(m.path);
            if (cur != 0 && cur != m.mtime) { m.mtime = cur; m.loaded = false; }
        }
    }

    void enqueue(LoadRequest req) { queue_.push_back(std::move(req)); }
    std::vector<LoadRequest>& queue() noexcept { return queue_; }
    void clear_queue() noexcept { queue_.clear(); }

    std::size_t count() const noexcept { return metas_.size(); }

private:
    std::vector<AssetMeta> metas_;
    std::unordered_map<std::string, AssetId> path_to_id_;
    std::vector<LoadRequest> queue_;
};

} // namespace vengine::assets
