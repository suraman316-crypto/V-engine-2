#include <vengine/core/Assert.hpp>

#include <cstdio>
#include <cstdlib>

namespace vengine::core::detail {

[[noreturn]] void assertion_failed(const char* expr, const char* file, int line,
                                   const char* func, const char* msg) noexcept {
    VENGINE_LOG_FATAL("Assert", "ASSERTION FAILED: (%s) %s\n  at %s:%d in %s",
                      expr, (msg ? msg : ""), file, line, func);
    std::fflush(stdout);
    std::abort();
}

} // namespace vengine::core::detail
