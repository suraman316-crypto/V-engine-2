#pragma once

#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>

#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace vengine::core {

/// A typed configuration value. The engine uses this for project settings,
/// quality presets, and runtime options. Values are strongly typed to avoid
/// the "everything is a string" footgun.
using ConfigValue = std::variant<std::monostate, bool, i64, f64, std::string>;

/// Thread-safe key/value configuration store with atomic persistence.
///
/// Persistence is written atomically (temp file + rename) so a crash during
/// save never corrupts the existing config file (per README rule #11).
class Config {
public:
    Config() = default;

    // ---- In-memory access ---------------------------------------------------
    void        set(std::string_view key, ConfigValue value);
    ConfigValue get(std::string_view key, ConfigValue fallback = {}) const;

    bool   get_bool  (std::string_view key, bool   fallback = false) const;
    i64    get_int   (std::string_view key, i64    fallback = 0) const;
    f64    get_float (std::string_view key, f64    fallback = 0.0) const;
    std::string get_string(std::string_view key, std::string_view fallback = "") const;

    bool has(std::string_view key) const;
    void erase(std::string_view key);
    void clear();

    // ---- Persistence --------------------------------------------------------
    /// Load from a simple INI-like file. Missing file is not an error.
    Result<void> load(std::string_view path);
    /// Atomically write the config to disk.
    Result<void> save(std::string_view path) const;

    // Snapshot of all keys (sorted) — for the editor/project settings UI.
    std::vector<std::pair<std::string, ConfigValue>> entries() const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, ConfigValue> values_;
};

const char* config_value_type_name(const ConfigValue& v) noexcept;

} // namespace vengine::core
