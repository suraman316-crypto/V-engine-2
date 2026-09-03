#pragma once

// A minimal, portable source_location substitute compatible with C++20.

#include <cstdint>
#include <string_view>

namespace vengine::core {

struct SourceLocation {
    std::string_view file;
    std::uint32_t    line{};
    std::string_view function;

    static constexpr SourceLocation current(
        std::string_view file = __builtin_FILE(),
        std::uint32_t    line = __builtin_LINE(),
        std::string_view func = __builtin_FUNCTION()) {
        return SourceLocation{file, line, func};
    }
};

} // namespace vengine::core
