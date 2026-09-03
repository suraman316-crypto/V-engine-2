#pragma once

// Compiler/OS detection and portable feature macros shared by the whole engine.

#if defined(__ANDROID__)
#  define VENGINE_PLATFORM_ANDROID 1
#else
#  define VENGINE_PLATFORM_ANDROID 0
#endif

#if defined(_WIN32)
#  define VENGINE_PLATFORM_WINDOWS 1
#else
#  define VENGINE_PLATFORM_WINDOWS 0
#endif

#if defined(__linux__) && !VENGINE_PLATFORM_ANDROID
#  define VENGINE_PLATFORM_LINUX 1
#else
#  define VENGINE_PLATFORM_LINUX 0
#endif

#if defined(__APPLE__)
#  define VENGINE_PLATFORM_APPLE 1
#else
#  define VENGINE_PLATFORM_APPLE 0
#endif

#if defined(__EMSCRIPTEN__)
#  define VENGINE_PLATFORM_WEB 1
#else
#  define VENGINE_PLATFORM_WEB 0
#endif

// Export/import for the shared engine library on the desktop side.
#if defined(VENGINE_SHARED)
#  if defined(_MSC_VER)
#    define VENGINE_API __declspec(dllexport)
#  else
#    define VENGINE_API __attribute__((visibility("default")))
#  endif
#else
#  define VENGINE_API
#endif
