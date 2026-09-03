#pragma once

// V Engine 2.0 — Mat3: 3x3 matrix for 2D affine transforms and rotations.
#include <vengine/math/Vec2.hpp>
#include <vengine/math/Vec3.hpp>

namespace vengine::math {

struct Mat3 {
    float m[9]{
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
    };

    static Mat3 identity() { return {}; }

    static Mat3 translation(const Vec2f& t) {
        Mat3 r;
        r.m[2] = t.x; r.m[5] = t.y;
        return r;
    }
    static Mat3 rotation(float radians) {
        float c = std::cos(radians), s = std::sin(radians);
        Mat3 r;
        r.m[0] = c; r.m[1] = -s;
        r.m[3] = s; r.m[4] = c;
        return r;
    }
    static Mat3 scaling(const Vec2f& s) {
        Mat3 r;
        r.m[0] = s.x; r.m[4] = s.y;
        return r;
    }

    Mat3 operator*(const Mat3& o) const {
        Mat3 r;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                float sum = 0;
                for (int k = 0; k < 3; ++k) sum += m[i*3+k] * o.m[k*3+j];
                r.m[i*3+j] = sum;
            }
        return r;
    }

    Vec2f transform_point(const Vec2f& p) const {
        return {m[0]*p.x + m[1]*p.y + m[2],
                m[3]*p.x + m[4]*p.y + m[5]};
    }
    Vec2f transform_vector(const Vec2f& v) const {
        return {m[0]*v.x + m[1]*v.y, m[3]*v.x + m[4]*v.y};
    }

    float determinant() const {
        return m[0]*(m[4]*m[8] - m[5]*m[7])
             - m[1]*(m[3]*m[8] - m[5]*m[6])
             + m[2]*(m[3]*m[7] - m[4]*m[6]);
    }
    Mat3 inverse() const {
        float det = determinant();
        if (std::abs(det) < 1e-9f) return identity();
        float inv_det = 1.0f / det;
        Mat3 r;
        r.m[0] = (m[4]*m[8] - m[5]*m[7]) * inv_det;
        r.m[1] = (m[2]*m[7] - m[1]*m[8]) * inv_det;
        r.m[2] = (m[1]*m[5] - m[2]*m[4]) * inv_det;
        r.m[3] = (m[5]*m[6] - m[3]*m[8]) * inv_det;
        r.m[4] = (m[0]*m[8] - m[2]*m[6]) * inv_det;
        r.m[5] = (m[2]*m[3] - m[0]*m[5]) * inv_det;
        r.m[6] = (m[3]*m[7] - m[4]*m[6]) * inv_det;
        r.m[7] = (m[1]*m[6] - m[0]*m[7]) * inv_det;
        r.m[8] = (m[0]*m[4] - m[1]*m[3]) * inv_det;
        return r;
    }
};

} // namespace vengine::math
