#include <vengine/core/Config.hpp>
#include <vengine/core/Logging.hpp>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

namespace vengine::core {

namespace {

// Atomic write: write to "<path>.tmp" then rename over the target.
Result<void> atomic_write_text(std::string_view path, const std::string& text) {
    std::string tmp{path};
    tmp += ".tmp";

    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) {
        return Result<void>::Fail(ErrorCode::IoFailure,
            "cannot open temporary file for write", RecoveryHint::CheckPermissions);
    }
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    out.flush();
    if (!out) {
        return Result<void>::Fail(ErrorCode::IoFailure, "write failed");
    }
    out.close();

    // std::rename is atomic on POSIX when both paths are on the same filesystem.
    if (std::rename(tmp.c_str(), std::string(path).c_str()) != 0) {
        std::remove(tmp.c_str());
        return Result<void>::Fail(ErrorCode::IoFailure,
            "rename failed: " + std::string(std::strerror(errno)),
            RecoveryHint::CheckPermissions);
    }
    return Result<void>::Ok();
}

std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        if (c == '\\' || c == '"') { out.push_back('\\'); out.push_back(c); }
        else if (c == '\n')        { out += "\\n"; }
        else                        { out.push_back(c); }
    }
    return out;
}

std::string unescape(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (usize i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[i + 1];
            if (n == 'n')      out.push_back('\n');
            else if (n == '"') out.push_back('"');
            else if (n == '\\') out.push_back('\\');
            else               { out.push_back('\\'); out.push_back(n); }
            ++i;
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

} // namespace

const char* config_value_type_name(const ConfigValue& v) noexcept {
    return std::visit([](auto&& val) -> const char* {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::monostate>) return "none";
        else if constexpr (std::is_same_v<T, bool>)      return "bool";
        else if constexpr (std::is_same_v<T, i64>)       return "int";
        else if constexpr (std::is_same_v<T, f64>)       return "float";
        else if constexpr (std::is_same_v<T, std::string>) return "string";
        else return "unknown";
    }, v);
}

void Config::set(std::string_view key, ConfigValue value) {
    const std::lock_guard lock(mutex_);
    values_[std::string(key)] = std::move(value);
}

ConfigValue Config::get(std::string_view key, ConfigValue fallback) const {
    const std::lock_guard lock(mutex_);
    auto it = values_.find(std::string(key));
    if (it == values_.end()) return fallback;
    return it->second;
}

bool Config::get_bool(std::string_view key, bool fallback) const {
    auto v = get(key);
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v);
    if (std::holds_alternative<i64>(v))  return std::get<i64>(v) != 0;
    return fallback;
}

i64 Config::get_int(std::string_view key, i64 fallback) const {
    auto v = get(key);
    if (std::holds_alternative<i64>(v)) return std::get<i64>(v);
    if (std::holds_alternative<f64>(v)) return static_cast<i64>(std::get<f64>(v));
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? 1 : 0;
    return fallback;
}

f64 Config::get_float(std::string_view key, f64 fallback) const {
    auto v = get(key);
    if (std::holds_alternative<f64>(v)) return std::get<f64>(v);
    if (std::holds_alternative<i64>(v)) return static_cast<f64>(std::get<i64>(v));
    return fallback;
}

std::string Config::get_string(std::string_view key, std::string_view fallback) const {
    auto v = get(key);
    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
    return std::string(fallback);
}

bool Config::has(std::string_view key) const {
    const std::lock_guard lock(mutex_);
    return values_.find(std::string(key)) != values_.end();
}

void Config::erase(std::string_view key) {
    const std::lock_guard lock(mutex_);
    values_.erase(std::string(key));
}

void Config::clear() {
    const std::lock_guard lock(mutex_);
    values_.clear();
}

Result<void> Config::load(std::string_view path) {
    std::ifstream in{std::string(path)};
    if (!in) {
        // Missing file is fine — start empty.
        return Result<void>::Ok();
    }

    Config loaded;
    std::string line;
    std::size_t lineno = 0;
    while (std::getline(in, line)) {
        ++lineno;
        auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        // trim
        auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        auto last = line.find_last_not_of(" \t\r\n");
        line = line.substr(first, last - first + 1);

        auto eq = line.find('=');
        if (eq == std::string::npos) {
            VENGINE_LOG_WARN("Config", "skipping malformed line %zu in %.*s",
                             lineno, static_cast<int>(path.size()), path.data());
            continue;
        }
        std::string key = line.substr(0, eq);
        std::string raw = line.substr(eq + 1);
        // trim key
        auto kf = key.find_first_not_of(" \t");
        auto kl = key.find_last_not_of(" \t");
        if (kf == std::string::npos) continue;
        key = key.substr(kf, kl - kf + 1);

        // trim raw
        auto rf = raw.find_first_not_of(" \t");
        if (rf != std::string::npos) {
            auto rl = raw.find_last_not_of(" \t");
            raw = raw.substr(rf, rl - rf + 1);
        } else raw.clear();

        ConfigValue value;
        if (!raw.empty() && raw.front() == '"') {
            // quoted string
            auto end = raw.find_last_of('"');
            if (raw.size() >= 2 && end != 0) {
                std::string_view inner{raw.data() + 1, end - 1};
                value = unescape(inner);
            } else {
                value = std::string{};
            }
        } else if (raw == "true")  { value = true; }
        else if (raw == "false")   { value = false; }
        else if (raw.empty())      { value = std::monostate{}; }
        else {
            // try int, then float
            bool all_int = true;
            bool has_dot = false;
            for (char c : raw) {
                if (c == '-' || c == '+') continue;
                if (c == '.') { has_dot = true; continue; }
                if (c < '0' || c > '9') { all_int = false; break; }
            }
            if (!all_int) {
                value = raw; // treat as string fallback
            } else if (has_dot) {
                try { value = std::stod(raw); } catch (...) { value = raw; }
            } else {
                try { value = static_cast<i64>(std::stoll(raw)); } catch (...) { value = raw; }
            }
        }
        loaded.set(key, value);
    }

    // Adopt the loaded map. Config holds a non-movable mutex, so we swap the
    // data under our own lock rather than move-assigning the whole object.
    auto loaded_entries = loaded.entries();
    {
        const std::lock_guard lock(mutex_);
        values_.clear();
        for (auto& [k, v] : loaded_entries) values_.emplace(std::move(k), std::move(v));
    }
    return Result<void>::Ok();
}

Result<void> Config::save(std::string_view path) const {
    std::ostringstream ss;
    ss << "# V Engine project configuration\n";
    for (const auto& [k, v] : entries()) {
        ss << k << " = ";
        if (std::holds_alternative<bool>(v))         ss << (std::get<bool>(v) ? "true" : "false");
        else if (std::holds_alternative<i64>(v))     ss << std::get<i64>(v);
        else if (std::holds_alternative<f64>(v))     ss << std::get<f64>(v);
        else if (std::holds_alternative<std::string>(v)) ss << '"' << escape(std::get<std::string>(v)) << '"';
        else ss << "";
        ss << '\n';
    }
    return atomic_write_text(path, ss.str());
}

std::vector<std::pair<std::string, ConfigValue>> Config::entries() const {
    const std::lock_guard lock(mutex_);
    std::vector<std::pair<std::string, ConfigValue>> out;
    out.reserve(values_.size());
    for (const auto& [k, v] : values_) out.emplace_back(k, v);
    return out;
}

} // namespace vengine::core
