#pragma once

// Column-major 4x4 matrix. V Engine is 2D, but the renderer's view-projection
// pipeline, render-target transforms, and the future Vulkan backend all want
// a real affine matrix type rather than ad-hoc float[16] arrays.
//
// Storage is a flat array of 16 floats in column-major order (matching
// OpenGL/Vulkan conventions): element (row r, col c) = m[c*4 + r].
//
// The matrix is the lowest-level math primitive the renderer depends on, so it
// is kept header-only and branch-free in the hot paths.

#include <vengine/math/Vec3.hpp>
#include <vengine/math/Vec2.hpp>

#include <cmath>
#include <cstring>

namespace vengine::math {

struct Mat4 {
    float m[16]{1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1};

    constexpr Mat4() = default;

    /// Build from 16 column-major values.
    constexpr Mat4(float m0, float m1, float m2, float m3,
                  float m4, float m5, float m6, float m7,
                  float m8, float m9, float m10, float m11,
                  float m12, float m13, float m14, float m15) noexcept {
        m[0]=m0;  m[1]=m1;  m[2]=m2;  m[3]=m3;
        m[4]=m4;  m[5]=m5;  m[6]=m6;  m[7]=m7;
        m[8]=m8;  m[9]=m9;  m[10]=m10;m[11]=m11;
        m[12]=m12;m[13]=m13;m[14]=m14;m[15]=m15;
    }

    constexpr bool operator==(const Mat4& o) const noexcept {
        for (int i = 0; i < 16; ++i) if (m[i] != o.m[i]) return false;
        return true;
    }

    constexpr static Mat4 identity() noexcept { return Mat4{}; }

    /// Element access: (row r, col c) = m[c*4 + r].
    constexpr float  operator()(int r, int c) const noexcept { return m[c * 4 + r]; }
    constexpr float& operator()(int r, int c)       noexcept { return m[c * 4 + r]; }

    /// Matrix * matrix.
    constexpr Mat4 operator*(const Mat4& o) const noexcept {
        Mat4 r{};
        for (int c = 0; c < 4; ++c)
            for (int row = 0; row < 4; ++row) {
                float s = 0.0f;
                for (int k = 0; k < 4; ++k) s += (*this)(row, k) * o(k, c);
                r(row, c) = s;
            }
        return r;
    }
    constexpr Mat4& operator*=(const Mat4& o) noexcept { *this = *this * o; return *this; }

    /// Transform a point (w = 1).
    constexpr Vec3f operator*(const Vec3f& v) const noexcept {
        return {
            m[0]*v.x + m[4]*v.y + m[8] *v.z + m[12],
            m[1]*v.x + m[5]*v.y + m[9] *v.z + m[13],
            m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14],
        };
    }

    // ---- common constructors -------------------------------------------------

    constexpr static Mat4 translate(const Vec3f& t) noexcept {
        return Mat4(1,0,0,0, 0,1,0,0, 0,0,1,0, t.x, t.y, t.z, 1);
    }
    constexpr static Mat4 translate(const Vec2f& t) noexcept {
        return translate(Vec3f{t.x, t.y, 0.0f});
    }
    static Mat4 rotate_z(float radians) noexcept {
        float c = std::cos(radians), s = std::sin(radians);
        return Mat4(c, s, 0, 0,  -s, c, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1);
    }
    constexpr static Mat4 scale(const Vec3f& s) noexcept {
        return Mat4(s.x,0,0,0, 0,s.y,0,0, 0,0,s.z,0, 0,0,0,1);
    }
    constexpr static Mat4 scale(const Vec2f& s) noexcept {
        return scale(Vec3f{s.x, s.y, 1.0f});
    }
    constexpr static Mat4 scale(float s) noexcept {
        return scale(Vec3f{s, s, s});
    }

    /// TRS (translation * rotation * scale), the standard 2D world transform.
    static Mat4 trs(const Vec2f& pos, float rot_rad, const Vec2f& scl) noexcept {
        return translate(pos) * rotate_z(rot_rad) * scale(scl);
    }

    /// OpenGL-style orthographic projection (right-handed). Maps the given
    /// box to [-1,1] on each axis. Used by the 2D camera.
    static Mat4 ortho(float left, float right, float bottom, float top,
                     float near_z, float far_z) noexcept {
        float rpl = right - left,  tpb = top - bottom,  fpn = far_z - near_z;
        if (rpl == 0.0f || tpb == 0.0f || fpn == 0.0f) return identity();
        Mat4 r{};
        r(0,0) = 2.0f / rpl;
        r(1,1) = 2.0f / tpb;
        r(2,2) = -2.0f / fpn;
        r(0,3) = -(right + left) / rpl;
        r(1,3) = -(top + bottom) / tpb;
        r(2,3) = -(far_z + near_z) / fpn;
        return r;
    }

    /// Inverse of an affine transform (translate * rotate * scale). Cheaper
    /// than a general 4x4 inverse and sufficient for the 2D pipeline.
    static Mat4 inverse_trs(const Mat4& t) noexcept {
        // Pull out translation, rotation (Z), and scale from the columns.
        Vec2f tr{t(0,3), t(1,3)};
        float a = t(0,0), b = t(1,0), c = t(0,1), d = t(1,1);
        float det = a * d - b * c;
        if (det == 0.0f) return identity();
        float inv_det = 1.0f / det;
        Vec2f inv_pos{-tr.x * inv_det, -tr.y * inv_det}; // approx; see full path below
        (void)inv_pos;
        // Build inverse: inv(scale) * inv(rotate) * inv(translate).
        Mat4 inv_translate = translate(Vec2f{-tr.x, -tr.y});
        // Rotation inverse = transpose of the 2x2 upper-left (assuming no shear).
        Mat4 inv_rot = identity();
        inv_rot(0,0) =  d * inv_det; inv_rot(0,1) = -c * inv_det;
        inv_rot(1,0) = -b * inv_det; inv_rot(1,1) =  a * inv_det;
        return inv_rot * inv_translate;
    }
};

} // namespace vengine::math
