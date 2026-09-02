#pragma once

#include <vengine/core/Macros.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/core/SourceLocation.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace vengine::core {

/// Every major engine API reports failure through this typed error, never by
/// silently returning a sentinel. Errors carry a stable code, a human-readable
/// message, and an optional recovery hint.
enum class ErrorCode : std::uint32_t {
    Ok = 0,
    // Generic
    Unknown,
    InvalidArgument,
    InvalidState,
    NotSupported,
    Cancelled,
    Timeout,
    // I/O / files
    FileNotFound,
    PermissionDenied,
    IoFailure,
    // Assets
    AssetNotFound,
    AssetCorrupted,
    UnsupportedFormat,
    // GPU / rendering
    ShaderCompilationFailed,
    TextureCreationFailed,
    GpuContextLost,
    // Serialization / scenes
    SerializationFailed,
    DeserializationFailed,
    InvalidScene,
    InvalidComponent,
    // Memory
    OutOfMemory,
    // Build
    BuildFailed,
};

/// Convert a code to a short, stable identifier (e.g. "AssetNotFound").
const char* error_code_name(ErrorCode code) noexcept;

/// Suggested recovery action for an error.
enum class RecoveryHint {
    None,
    Retry,
    ReimportAsset,
    CheckPermissions,
    FreeMemory,
    RecreateSurface,
    ContactSupport,
};

/// A rich, copyable error value.
struct Error {
    ErrorCode     code{ErrorCode::Unknown};
    std::string   message;
    std::string   detail;       ///< Optional extra context (path, etc.).
    RecoveryHint  hint{RecoveryHint::None};

    Error() = default;
    explicit Error(ErrorCode c, std::string msg = {}, RecoveryHint h = RecoveryHint::None)
        : code(c), message(std::move(msg)), hint(h) {}

    bool ok() const noexcept { return code == ErrorCode::Ok; }
    explicit operator bool() const noexcept { return !ok(); } // true == error present

    std::string format() const;
    static Error from(ErrorCode c, std::string msg = {}, RecoveryHint h = RecoveryHint::None) {
        Error e;
        e.code = c;
        e.message = std::move(msg);
        e.hint = h;
        return e;
    }
};

/// A Result<T> carries either a value or an Error. Use VENGINE_TRY to unwrap.
template <typename T>
class Result {
public:
    Result(T value) : value_(std::move(value)) {}
    Result(Error err) : error_(std::move(err)) {}

    static Result Ok(T v) { return Result{std::move(v)}; }
    static Result Fail(Error e) { return Result{std::move(e)}; }
    static Result Fail(ErrorCode c, std::string msg = {}, RecoveryHint h = RecoveryHint::None) {
        return Result{Error::from(c, std::move(msg), h)};
    }

    bool ok() const noexcept { return value_.has_value(); }
    explicit operator bool() const noexcept { return ok(); }

    const T& value() const { return *value_; }
    T&       value()       { return *value_; }
    T        take()        { return std::move(*value_); }

    const Error& error() const { return error_; }
    Error&       error()       { return error_; }

private:
    std::optional<T> value_;
    Error             error_{ErrorCode::Ok};
};

/// Specialization for void-returning operations: only success or error.
template <>
class Result<void> {
public:
    Result() = default;                       // success
    Result(Error err) : error_(std::move(err)) {}

    static Result Ok() { return Result{}; }
    static Result Fail(Error e) { return Result{std::move(e)}; }
    static Result Fail(ErrorCode c, std::string msg = {}, RecoveryHint h = RecoveryHint::None) {
        return Result{Error::from(c, std::move(msg), h)};
    }

    bool ok() const noexcept { return error_.code == ErrorCode::Ok; }
    explicit operator bool() const noexcept { return ok(); }
    const Error& error() const { return error_; }
    Error&       error()       { return error_; }

private:
    Error error_{ErrorCode::Ok};
};

// Macro helpers. VENGINE_TRY assigns the value or propagates the error.
#define VENGINE_TRY(var, expr) \
    auto _vengine_res_##var = (expr); \
    if (!_vengine_res_##var.ok()) return ::vengine::core::Result<decltype(_vengine_res_##var.value())>::Fail(_vengine_res_##var.error()); \
    auto var = std::move(_vengine_res_##var.value())

#define VENGINE_TRY_VOID(expr) \
    auto _vengine_res_ = (expr); \
    if (!_vengine_res_.ok()) return _vengine_res_.error()

} // namespace vengine::core
