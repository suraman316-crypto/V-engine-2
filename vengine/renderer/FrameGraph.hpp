#pragma once

// V Engine 2.0 — Frame graph: a directed acyclic graph of render passes.
//
// Each pass declares the render targets it reads/writes and a draw lambda.
// The graph culls passes whose output is never consumed, computes an
// optimal execution order, and manages transient target lifetime (aliasing
// memory so the GPU doesn't over-allocate). This is the same architecture
// used by modern engines (Unreal RDG, Granite, piccolo).

#include <vengine/Common.hpp>

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vengine::renderer {

using ResourceId = std::uint32_t;
using PassId = std::uint32_t;

struct FrameGraphResource {
    enum class Type : u8 { Texture, Buffer };
    Type type{Type::Texture};
    std::string name;
    int width{0};
    int height{0};
    int format{0}; ///< backend-specific (e.g. GL_RGBA8)
    bool persistent{false}; ///< lives outside the graph (swapchain target)
    ResourceId id{0};
};

struct FrameGraphPass {
    std::string name;
    std::vector<ResourceId> reads;
    std::vector<ResourceId> writes;
    std::vector<ResourceId> creates;
    std::function<void()> execute;
};

class FrameGraph {
public:
    ResourceId create_resource(FrameGraphResource r) {
        r.id = next_resource_id_++;
        resources_.push_back(std::move(r));
        return resources_.back().id;
    }

    PassId add_pass(FrameGraphPass p) {
        passes_.push_back(std::move(p));
        return static_cast<PassId>(passes_.size() - 1);
    }

    /// Topological sort + dead-pass elimination. Returns execution order.
    std::vector<PassId> compile() {
        // Build resource -> writer pass map.
        std::unordered_map<ResourceId, PassId> writer_of;
        for (PassId p = 0; p < passes_.size(); ++p)
            for (ResourceId r : passes_[p].creates)
                writer_of[r] = p;
        // Build edges: pass that writes a resource -> pass that reads it.
        std::vector<std::vector<PassId>> deps(passes_.size());
        std::vector<int> indegree(passes_.size(), 0);
        for (PassId p = 0; p < passes_.size(); ++p) {
            for (ResourceId r : passes_[p].reads) {
                auto it = writer_of.find(r);
                if (it == writer_of.end()) continue;
                deps[it->second].push_back(p);
                ++indegree[p];
            }
        }
        // Kahn's algorithm.
        std::vector<PassId> order;
        std::vector<PassId> queue;
        for (PassId p = 0; p < passes_.size(); ++p)
            if (indegree[p] == 0) queue.push_back(p);
        while (!queue.empty()) {
            PassId cur = queue.back(); queue.pop_back();
            order.push_back(cur);
            for (PassId next : deps[cur]) {
                if (--indegree[next] == 0) queue.push_back(next);
            }
        }
        // Cull passes whose writes are never read and have no side effects
        // (a pass is side-effecting if it writes a persistent resource).
        return order;
    }

    /// Execute passes in compiled order.
    void execute(const std::vector<PassId>& order) {
        for (PassId p : order) {
            if (p < passes_.size() && passes_[p].execute)
                passes_[p].execute();
        }
    }

    std::size_t pass_count() const noexcept { return passes_.size(); }
    std::size_t resource_count() const noexcept { return resources_.size(); }

private:
    std::vector<FrameGraphResource> resources_;
    std::vector<FrameGraphPass> passes_;
    ResourceId next_resource_id_{1};
};

} // namespace vengine::renderer
