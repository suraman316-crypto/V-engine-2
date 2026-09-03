#pragma once

#include <vengine/core/Macros.hpp>
#include <vengine/core/Types.hpp>

#include <cstddef>
#include <cstdio>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace vengine::core {

/// Severity of a log message.
enum class LogLevel : std::uint8_t {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    Count
};

/// Where a log line is emitted.
enum class LogSink : std::uint8_t {
    Stdout = 0,
    AndroidLogcat, ///< Android platform layer installs this sink.
    File,
    Custom
};

/// A single log record. Passed to user-installed sinks.
struct LogRecord {
    LogLevel   level{};
    LogSink    sink{};
    std::string tag;     ///< Short category, e.g. "Renderer".
    std::string message; ///< Pre-formatted text.
    std::string file;    ///< Source file (__FILE__).
    int         line{};  ///< Source line.
};

/// Sink callback signature. Receives a record; must not throw.
using LogSinkCallback = void (*)(const LogRecord& record, void* user_data);

/// Thread-safe logging facade. The engine owns a single global instance;
/// platform layers may install additional sinks (e.g. logcat).
///
/// Design goals:
///   * No heap allocation on the fast path when the message fits the small
///     buffer used by the variadic formatter.
///   * Never throws; logging failures are silently dropped (logging must not
///     crash the engine).
///   * Atomic per-call flush is not required; a single mutex serializes output.
class Logger {
public:
    static Logger& instance();

    /// Set the minimum level that is emitted. Messages below are dropped.
    void set_min_level(LogLevel level) noexcept;

    /// Install a custom sink. Pass nullptr to remove. Returns the previous one.
    LogSinkCallback install_sink(LogSinkCallback callback, void* user_data) noexcept;

    /// Open a file sink. Returns false on failure (path unwritable, etc.).
    bool open_file(std::string_view path) noexcept;
    void close_file() noexcept;

    /// Core entry point. Not normally called directly; use VENGINE_LOG_*.
    void log(LogLevel level, std::string_view tag, std::string_view message,
             std::string_view file, int line) noexcept;

    /// printf-style helper. Returns the number of bytes written to the
    /// internal buffer, or a negative value on formatting failure.
    int logf(LogLevel level, std::string_view tag, std::string_view file, int line,
             const char* fmt, ...) const noexcept VENGINE_PRINTF(6, 7);

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void write_to_sinks(const LogRecord& record) const noexcept;

    mutable std::mutex      mutex_;
    LogLevel                min_level_{LogLevel::Info};
    LogSinkCallback         custom_sink_{nullptr};
    void*                   custom_user_data_{nullptr};
    std::FILE*              file_{nullptr};
};

const char* level_name(LogLevel level) noexcept;
const char* level_short(LogLevel level) noexcept;

} // namespace vengine::core

// ---- Convenience macros ------------------------------------------------------
//
// Usage: VENGINE_LOG_INFO("Renderer", "Texture %s loaded", path);
//
#define VENGINE_LOG_TRACE(tag, ...) \
    ::vengine::core::Logger::instance().logf( \
        ::vengine::core::LogLevel::Trace, (tag), __FILE__, __LINE__, __VA_ARGS__)
#define VENGINE_LOG_DEBUG(tag, ...) \
    ::vengine::core::Logger::instance().logf( \
        ::vengine::core::LogLevel::Debug, (tag), __FILE__, __LINE__, __VA_ARGS__)
#define VENGINE_LOG_INFO(tag, ...) \
    ::vengine::core::Logger::instance().logf( \
        ::vengine::core::LogLevel::Info, (tag), __FILE__, __LINE__, __VA_ARGS__)
#define VENGINE_LOG_WARN(tag, ...) \
    ::vengine::core::Logger::instance().logf( \
        ::vengine::core::LogLevel::Warn, (tag), __FILE__, __LINE__, __VA_ARGS__)
#define VENGINE_LOG_ERROR(tag, ...) \
    ::vengine::core::Logger::instance().logf( \
        ::vengine::core::LogLevel::Error, (tag), __FILE__, __LINE__, __VA_ARGS__)
#define VENGINE_LOG_FATAL(tag, ...) \
    ::vengine::core::Logger::instance().logf( \
        ::vengine::core::LogLevel::Fatal, (tag), __FILE__, __LINE__, __VA_ARGS__)
