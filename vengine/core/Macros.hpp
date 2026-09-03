#pragma once

#include <vengine/core/Platform.hpp>

// printf-format attribute for the compiler to check variadic log calls.
#if defined(__GNUC__) || defined(__clang__)
#  define VENGINE_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#  define VENGINE_PRINTF(fmt, args)
#endif

// Mark a function/variable as deprecated with a message.
#if defined(__GNUC__) || defined(__clang__)
#  define VENGINE_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#  define VENGINE_DEPRECATED(msg) __declspec(deprecated(msg))
#else
#  define VENGINE_DEPRECATED(msg)
#endif

// Unreachable hint for the optimizer.
#if defined(__GNUC__) || defined(__clang__)
#  define VENGINE_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
#  define VENGINE_UNREACHABLE() __assume(0)
#else
#  define VENGINE_UNREACHABLE() std::abort()
#endif

// Concatenate preprocessor tokens.
#define VENGINE_PRAGMA(x) _Pragma(#x)
#define VENGINE_STRINGIFY_(x) #x
#define VENGINE_STRINGIFY(x) VENGINE_STRINGIFY_(x)
