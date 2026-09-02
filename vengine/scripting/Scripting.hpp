#pragma once

#include <vengine/Common.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/scene/Scene.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace vengine::scripting {

/// Script binding API surface. The engine exposes a small, safe scripting
/// surface (the concrete VM — e.g. Lua/QuickJS — is added in a later phase);
/// the interface here is what game logic binds against regardless of VM.
class IScriptRuntime {
public:
    virtual ~IScriptRuntime() = default;

    virtual Result<void> initialize() = 0;
    virtual void         shutdown() = 0;

    /// Register a per-frame system written in script source. Compiled/cached.
    virtual Result<void> add_system(std::string_view name, std::string_view source) = 0;
    /// Run all registered systems for one frame.
    virtual void         tick(scene::Scene& scene, float dt) = 0;
};

} // namespace vengine::scripting
