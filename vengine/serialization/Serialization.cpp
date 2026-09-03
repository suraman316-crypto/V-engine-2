#include <vengine/serialization/Serialization.hpp>
#include <vengine/core/Logging.hpp>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <utility>

namespace vengine::serialization {

// ---- JsonValue accessors ----------------------------------------------------

bool JsonValue::as_bool(bool fallback) const {
    return type_ == Type::Bool ? b_ : fallback;
}
i64 JsonValue::as_int(i64 fallback) const {
    if (type_ == Type::Int)   return i_;
    if (type_ == Type::Float) return static_cast<i64>(f_);
    return fallback;
}
double JsonValue::as_float(double fallback) const {
    if (type_ == Type::Float) return f_;
    if (type_ == Type::Int)   return static_cast<double>(i_);
    return fallback;
}
std::string JsonValue::as_string(std::string_view fallback) const {
    return type_ == Type::String ? s_ : std::string(fallback);
}

const JsonValue* JsonValue::find(std::string_view key) const {
    if (type_ != Type::Object) return nullptr;
    for (const auto& [k, v] : o_) {
        if (k == key) return &v;
    }
    return nullptr;
}

JsonValue& JsonValue::set(std::string key, JsonValue v) {
    if (type_ != Type::Object) { type_ = Type::Object; o_.clear(); }
    for (auto& [k, existing] : o_) {
        if (k == key) { existing = std::move(v); return existing; }
    }
    o_.emplace_back(std::move(key), std::move(v));
    return o_.back().second;
}

// ---- Atomic file write ------------------------------------------------------

Result<void> atomic_write_text(std::string_view path, std::string_view text) {
    std::string tmp{path};
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) {
            return Result<void>::Fail(ErrorCode::IoFailure,
                "cannot open temp file for write", RecoveryHint::CheckPermissions);
        }
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        out.flush();
        if (!out) {
            return Result<void>::Fail(ErrorCode::IoFailure, "write failed");
        }
    }
    if (std::rename(tmp.c_str(), std::string(path).c_str()) != 0) {
        std::remove(tmp.c_str());
        return Result<void>::Fail(ErrorCode::IoFailure,
            "atomic rename failed: " + std::string(std::strerror(errno)),
            RecoveryHint::CheckPermissions);
    }
    return Result<void>::Ok();
}

// ---- JSON parser (recursive descent) ---------------------------------------

namespace {

struct Parser {
    std::string_view s;
    std::size_t i{0};

    [[nodiscard]] bool eof() const { return i >= s.size(); }

    void skip_ws() {
        while (!eof()) {
            char c = s[i];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ++i;
            else break;
        }
    }

    char peek() { return eof() ? '\0' : s[i]; }
    char next() { return eof() ? '\0' : s[i++]; }

    Error fail(std::string msg) {
        return Error::from(ErrorCode::DeserializationFailed,
            msg + " at offset " + std::to_string(i));
    }

    Result<JsonValue> parse_value() {
        skip_ws();
        if (eof()) return fail("unexpected end of input");
        char c = peek();
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return parse_string();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == 'n') return parse_null();
        if (c == '-' || (c >= '0' && c <= '9')) return parse_number();
        return fail(std::string("unexpected character '") + c + "'");
    }

    Result<JsonValue> parse_object() {
        ++i; // {
        JsonValue v = JsonValue::make_object();
        skip_ws();
        if (peek() == '}') { ++i; return v; }
        for (;;) {
            skip_ws();
            if (peek() != '"') return fail("expected string key in object");
            auto key_res = parse_string();
            if (!key_res.ok()) return key_res.error();
            std::string key = key_res.value().as_string();
            skip_ws();
            if (next() != ':') return fail("expected ':' after key");
            auto val_res = parse_value();
            if (!val_res.ok()) return val_res.error();
            v.set(std::move(key), std::move(val_res.value()));
            skip_ws();
            char c = next();
            if (c == ',') continue;
            if (c == '}') break;
            return fail("expected ',' or '}' in object");
        }
        return v;
    }

    Result<JsonValue> parse_array() {
        ++i; // [
        JsonValue v = JsonValue::make_array();
        v.array().reserve(4);
        skip_ws();
        if (peek() == ']') { ++i; return v; }
        for (;;) {
            auto item = parse_value();
            if (!item.ok()) return item.error();
            v.array().push_back(std::move(item.value()));
            skip_ws();
            char c = next();
            if (c == ',') continue;
            if (c == ']') break;
            return fail("expected ',' or ']' in array");
        }
        return v;
    }

    Result<JsonValue> parse_string() {
        ++i; // opening quote
        std::string out;
        out.reserve(16);
        while (!eof()) {
            char c = next();
            if (c == '"') return JsonValue{std::move(out)};
            if (c == '\\') {
                if (eof()) return fail("trailing backslash in string");
                char e = next();
                switch (e) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        // Decode a 4-hex UTF-16 code unit into UTF-8. We handle
                        // the BMP case and surrogate pairs minimally; surrogate
                        // pairs are accepted but encoded as the replacement
                        // path for robustness.
                        std::string hex;
                        for (int k = 0; k < 4; ++k) {
                            if (eof()) return fail("incomplete \\u escape");
                            hex.push_back(next());
                        }
                        std::uint32_t cp = 0;
                        for (char h : hex) {
                            cp <<= 4;
                            if (h >= '0' && h <= '9') cp |= h - '0';
                            else if (h >= 'a' && h <= 'f') cp |= h - 'a' + 10;
                            else if (h >= 'A' && h <= 'F') cp |= h - 'A' + 10;
                            else return fail("invalid hex in \\u escape");
                        }
                        if (cp < 0x80) {
                            out.push_back(static_cast<char>(cp));
                        } else if (cp < 0x800) {
                            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                        } else {
                            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                        }
                        break;
                    }
                    default: return fail(std::string("invalid escape '\\") + e + "'");
                }
            } else {
                out.push_back(c);
            }
        }
        return fail("unterminated string");
    }

    Result<JsonValue> parse_bool() {
        if (s.substr(i, 4) == "true")  { i += 4; return JsonValue{true}; }
        if (s.substr(i, 5) == "false") { i += 5; return JsonValue{false}; }
        return fail("invalid literal");
    }

    Result<JsonValue> parse_null() {
        if (s.substr(i, 4) == "null") { i += 4; return JsonValue{}; }
        return fail("invalid literal");
    }

    Result<JsonValue> parse_number() {
        std::size_t start = i;
        bool is_float = false;
        if (peek() == '-') ++i;
        while (!eof()) {
            char c = peek();
            if (c >= '0' && c <= '9') { ++i; continue; }
            if (c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') {
                is_float = true; ++i; continue;
            }
            break;
        }
        std::string num{s.substr(start, i - start)};
        try {
            if (is_float) return JsonValue{std::stod(num)};
            return JsonValue{static_cast<i64>(std::stoll(num))};
        } catch (...) {
            return fail("invalid number '" + num + "'");
        }
    }
};

void dump_string(std::string& out, const std::string& s) {
    out.push_back('"');
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out.push_back(c);
                }
        }
    }
    out.push_back('"');
}

void dump_impl(std::string& out, const JsonValue& v) {
    switch (v.type()) {
        case JsonValue::Type::Null:   out += "null"; return;
        case JsonValue::Type::Bool:   out += v.as_bool() ? "true" : "false"; return;
        case JsonValue::Type::Int:    out += std::to_string(v.as_int()); return;
        case JsonValue::Type::Float:  out += std::to_string(v.as_float()); return;
        case JsonValue::Type::String: dump_string(out, v.as_string()); return;
        case JsonValue::Type::Array: {
            out.push_back('[');
            bool first = true;
            for (const auto& e : v.array()) {
                if (!first) out.push_back(',');
                first = false;
                dump_impl(out, e);
            }
            out.push_back(']');
            return;
        }
        case JsonValue::Type::Object: {
            out.push_back('{');
            bool first = true;
            for (const auto& [k, val] : v.object()) {
                if (!first) out.push_back(',');
                first = false;
                dump_string(out, k);
                out.push_back(':');
                dump_impl(out, val);
            }
            out.push_back('}');
            return;
        }
    }
}

} // namespace

Result<JsonValue> parse_json(std::string_view text) {
    Parser p{text, 0};
    auto res = p.parse_value();
    if (!res.ok()) return res.error();
    p.skip_ws();
    if (!p.eof()) {
        return Error::from(ErrorCode::DeserializationFailed,
            "trailing data at offset " + std::to_string(p.i));
    }
    return res;
}

std::string dump_json(const JsonValue& v) {
    std::string out;
    out.reserve(64);
    dump_impl(out, v);
    return out;
}

} // namespace vengine::serialization
