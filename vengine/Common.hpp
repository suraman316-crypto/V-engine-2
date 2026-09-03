#pragma once

// Convenience header: brings the core type aliases, Result/Error types, and a
// few macros into the top-level `vengine` namespace so that other modules
// (math, scene, renderer, ...) can refer to them unqualified without each
// having to spell out the `vengine::core::` prefix.

#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>

namespace vengine {

// Type aliases.
using core::i8;
using core::i16;
using core::i32;
using core::i64;
using core::u8;
using core::u16;
using core::u32;
using core::u64;
using core::f32;
using core::f64;
using core::uptr;
using core::usize;

// Error / Result surface.
using core::Error;
using core::ErrorCode;
using core::RecoveryHint;
using core::Result;

} // namespace vengine
