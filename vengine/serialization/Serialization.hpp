#pragma once

#include <vengine/Common.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace vengine::serialization {

/// A minimal, dependency-free JSON value model. Sufficient for engine
/// project/scene/asset metadata. Intentionally NOT a full JSON library —
/// it implements the subset the engine actually uses, with strict parsing
/// that rejects malformed input (README rule: prevent crashes from
/// corrupted serialization; never silently ignore errors).
class JsonValue {
public:
    enum class Type { Null, Bool, Int, Float, String, Array, Object };

    using Array  = std::vector<JsonValue>;
    using Object = std::vector<std::pair<std::string, JsonValue>>; // ordered

    JsonValue() = default;
    JsonValue(std::nullptr_t) {} // NOLINT: intentional null ctor
    JsonValue(bool b) : type_(Type::Bool), b_(b) {}
    JsonValue(i64 i)  : type_(Type::Int),  i_(i) {}
    JsonValue(int i)  : type_(Type::Int),  i_(static_cast<i64>(i)) {}
    JsonValue(double f) : type_(Type::Float), f_(f) {}
    JsonValue(const char* s) : type_(Type::String), s_(s) {}
    JsonValue(std::string s) : type_(Type::String), s_(std::move(s)) {}

    static JsonValue make_array()  { JsonValue v; v.type_ = Type::Array;  return v; }
    static JsonValue make_object() { JsonValue v; v.type_ = Type::Object; return v; }

    Type type() const noexcept { return type_; }
    bool is_null()   const noexcept { return type_ == Type::Null; }
    bool is_object() const noexcept { return type_ == Type::Object; }
    bool is_array()  const noexcept { return type_ == Type::Array; }

    bool        as_bool(bool fallback = false)   const;
    i64         as_int(i64 fallback = 0)         const;
    double      as_float(double fallback = 0.0)  const;
    std::string as_string(std::string_view fallback = "") const;

    Array& array() { return a_; }
    const Array& array() const { return a_; }

    Object& object() { return o_; }
    const Object& object() const { return o_; }

    /// Object access by key. Returns nullptr if missing or not an object.
    const JsonValue* find(std::string_view key) const;
    JsonValue&       set(std::string key, JsonValue v);

private:
    Type        type_{Type::Null};
    bool        b_{false};
    i64         i_{0};
    double      f_{0.0};
    std::string s_;
    Array       a_;
    Object      o_;
};

/// Parse a JSON string. On error, returns a failed Result whose error code is
/// DeserializationFailed with a message containing the position.
Result<JsonValue> parse_json(std::string_view text);

/// Serialize a JsonValue to a compact string.
std::string dump_json(const JsonValue& v);

/// Atomically write text to a path (temp + rename). Used for all project,
/// scene, and asset metadata writes (README rule #11).
Result<void> atomic_write_text(std::string_view path, std::string_view text);

} // namespace vengine::serialization
