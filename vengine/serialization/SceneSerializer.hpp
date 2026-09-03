#pragma once

#include <vengine/Common.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/scene/Scene.hpp>
#include <vengine/components/Components.hpp>
#include <vengine/serialization/Serialization.hpp>

#include <string>
#include <string_view>

namespace vengine::serialization {

/// Serialize a scene to a JSON string. Only core built-in components are
/// persisted here; user/script components can register custom (de)serializers
/// in a later phase.
std::string serialize_scene(const scene::Scene& scene);

/// Parse a scene JSON string into an existing scene (clears it first).
/// Reports DeserializationFailed on malformed input and InvalidScene when the
/// structure is valid JSON but not a valid V Engine scene.
Result<void> deserialize_scene(std::string_view text, scene::Scene& out);

/// Atomically save a scene to disk.
Result<void> save_scene(std::string_view path, const scene::Scene& scene);

/// Load a scene from disk into `out`. File-not-found is an explicit error.
Result<void> load_scene(std::string_view path, scene::Scene& out);

} // namespace vengine::serialization
