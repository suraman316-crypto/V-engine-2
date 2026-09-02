#include <vengine/core/Logging.hpp>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <utility>

namespace vengine::core {

namespace {
constexpr usize kFormatBuf = 1024; // inline, no heap on the fast path.

const char* const kLevelNames[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};
const char* const kLevelShort = "TDIWEF";
} // namespace

const char* level_name(LogLevel level) noexcept {
    const auto i = static_cast<usize>(level);
    if (i < static_cast<usize>(LogLevel::Count)) return kLevelNames[i];
    return "??????";
}

const char* level_short(LogLevel level) noexcept {
    const auto i = static_cast<usize>(level);
    if (i < static_cast<usize>(LogLevel::Count)) return &kLevelShort[i];
    return "?";
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::set_min_level(LogLevel level) noexcept {
    const std::lock_guard lock(mutex_);
    min_level_ = level;
}

LogSinkCallback Logger::install_sink(LogSinkCallback callback, void* user_data) noexcept {
    const std::lock_guard lock(mutex_);
    auto prev = custom_sink_;
    custom_sink_ = callback;
    custom_user_data_ = user_data;
    return prev;
}

bool Logger::open_file(std::string_view path) noexcept {
    const std::lock_guard lock(mutex_);
    if (file_) {
        std::fclose(file_);
        file_ = nullptr;
    }
    // path is not necessarily null-terminated; copy into a small buffer.
    char buf[1024];
    if (path.size() >= sizeof(buf)) return false;
    std::memcpy(buf, path.data(), path.size());
    buf[path.size()] = '\0';

    file_ = std::fopen(buf, "a");
    return file_ != nullptr;
}

void Logger::close_file() noexcept {
    const std::lock_guard lock(mutex_);
    if (file_) {
        std::fclose(file_);
        file_ = nullptr;
    }
}

void Logger::log(LogLevel level, std::string_view tag, std::string_view message,
                 std::string_view file, int line) noexcept {
    {
        const std::lock_guard lock(mutex_);
        if (static_cast<int>(level) < static_cast<int>(min_level_)) return;
    }

    LogRecord record{};
    record.level   = level;
    record.sink    = LogSink::Stdout;
    record.tag     = std::string(tag);
    record.message = std::string(message);
    record.file    = std::string(file);
    record.line    = line;

    write_to_sinks(record);
}

int Logger::logf(LogLevel level, std::string_view tag, std::string_view file, int line,
                 const char* fmt, ...) const noexcept {
    {
        const std::lock_guard lock(mutex_);
        if (static_cast<int>(level) < static_cast<int>(min_level_)) return 0;
    }

    char buf[kFormatBuf];
    va_list args;
    va_start(args, fmt);
    int n = std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n < 0) return n; // formatting error

    LogRecord record{};
    record.level   = level;
    record.sink    = LogSink::Stdout;
    record.tag     = std::string(tag);
    record.message.assign(buf, static_cast<usize>(n < 0 ? 0 : std::min(n, static_cast<int>(sizeof(buf) - 1))));
    record.file    = std::string(file);
    record.line    = line;

    write_to_sinks(record);
    return n;
}

void Logger::write_to_sinks(const LogRecord& record) const noexcept {
    // Build one formatted line: [HH:MM:SS][LEVEL][tag] message (file:line)
    char header[64];
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#if VENGINE_PLATFORM_WINDOWS
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    int hn = std::snprintf(header, sizeof(header), "%02d:%02d:%02d",
                           tm.tm_hour, tm.tm_min, tm.tm_sec);
    (void)hn;

    char line[kFormatBuf * 2];
    int n = std::snprintf(line, sizeof(line), "[%s][%-5s][%.*s] %.*s (%.*s:%d)\n",
                          header,
                          level_name(record.level),
                          static_cast<int>(record.tag.size()), record.tag.data(),
                          static_cast<int>(record.message.size()), record.message.data(),
                          static_cast<int>(record.file.size()), record.file.data(),
                          record.line);
    if (n <= 0) return;
    auto len = static_cast<usize>(n < static_cast<int>(sizeof(line)) ? n : static_cast<int>(sizeof(line) - 1));

    // Stdout sink.
    std::fwrite(line, 1, len, stdout);
    std::fflush(stdout);

    // File sink.
    {
        const std::lock_guard lock(mutex_);
        if (file_) {
            std::fwrite(line, 1, len, file_);
            std::fflush(file_);
        }
        // Custom sink (e.g. Android logcat). Invoked under lock to keep order.
        if (custom_sink_) {
            custom_sink_(record, custom_user_data_);
        }
    }
}

} // namespace vengine::core
