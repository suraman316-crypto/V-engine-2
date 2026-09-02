#pragma once

#include <vengine/core/Logging.hpp>
#include <vengine/core/Platform.hpp>
#include <vengine/core/SourceLocation.hpp>

#include <cstdlib>

namespace vengine::core {

/// Assertion policy:
///   VENGINE_ASSERT     - checked in all builds; failure is fatal.
///   VENGINE_DEBUG_ASSERT - checked only in debug builds; no cost in release.
///
/// Assertions must never be compiled out of release for *security-critical*
/// invariants (use VENGINE_ASSERT). Performance-critical, debug-only checks
/// use VENGINE_DEBUG_ASSERT.

namespace detail {
[[noreturn]] void assertion_failed(const char* expr, const char* file, int line,
                                   const char* func, const char* msg) noexcept;
}

} // namespace vengine::core

#define VENGINE_ASSERT(expr, ...) \
    do { \
        if (!(expr)) { \
            ::vengine::core::detail::assertion_failed( \
                #expr, __FILE__, __LINE__, __func__, "" __VA_ARGS__); \
        } \
    } while (false)

#define VENGINE_ASSERT_MSG(expr, msg) \
    do { \
        if (!(expr)) { \
            ::vengine::core::detail::assertion_failed( \
                #expr, __FILE__, __LINE__, __func__, msg); \
        } \
    } while (false)

#ifndef NDEBUG
#  define VENGINE_DEBUG_ASSERT(expr, ...) VENGINE_ASSERT(expr, ##__VA_ARGS__)
#  define VENGINE_DEBUG_ASSERT_MSG(expr, msg) VENGINE_ASSERT_MSG(expr, msg)
#else
#  define VENGINE_DEBUG_ASSERT(expr, ...) do {} while (false)
#  define VENGINE_DEBUG_ASSERT_MSG(expr, msg) do {} while (false)
#endif
