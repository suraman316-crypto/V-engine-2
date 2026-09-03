#include <vengine/math/AABB.hpp>
#include <vengine/math/Color.hpp>
#include <vengine/math/Easing.hpp>
#include <vengine/math/Mat4.hpp>
#include <vengine/math/Random.hpp>
#include <vengine/math/Rect.hpp>
#include <vengine/math/Transform2D.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/math/Vec3.hpp>

#include <gtest/gtest.h>

using namespace vengine::math;

TEST(Vec2, BasicArithmetic) {
    Vec2f a{1.0f, 2.0f}, b{3.0f, 4.0f};
    EXPECT_EQ(a + b, (Vec2f{4.0f, 6.0f}));
    EXPECT_EQ(b - a, (Vec2f{2.0f, 2.0f}));
    EXPECT_EQ(a * 2.0f, (Vec2f{2.0f, 4.0f}));
    EXPECT_FLOAT_EQ(a.dot(b), 11.0f);
    EXPECT_FLOAT_EQ(a.cross(b), -2.0f);
}

TEST(Vec2, LengthAndNormalize) {
    Vec2f v{3.0f, 4.0f};
    EXPECT_FLOAT_EQ(v.length(), 5.0f);
    EXPECT_FLOAT_EQ(v.length_squared(), 25.0f);
    Vec2f n = v.normalized();
    EXPECT_NEAR(n.length(), 1.0f, 1e-6);
    Vec2f zero{0,0};
    EXPECT_EQ(zero.normalized(), (Vec2f{0,0})); // zero stays zero
}

TEST(Color, PackUnpack) {
    Color c = Color::rgba(255, 128, 0, 64);
    auto packed = c.pack();
    Color back = Color::unpack(packed);
    EXPECT_NEAR(back.r, c.r, 1.0f/255.0f);
    EXPECT_NEAR(back.g, c.g, 1.0f/255.0f);
    EXPECT_NEAR(back.b, c.b, 1.0f/255.0f);
    EXPECT_NEAR(back.a, c.a, 1.0f/255.0f);
}

TEST(Rect, ContainsAndIntersects) {
    Rectf a{0,0,10,10};
    EXPECT_TRUE(a.contains(5,5));
    EXPECT_FALSE(a.contains(10,10));
    EXPECT_TRUE(a.intersects(Rectf{5,5,10,10}));
    EXPECT_FALSE(a.intersects(Rectf{20,20,5,5}));
}

TEST(Transform2D, ComposeIdentity) {
    Transform2D parent{{100,100}, 0.0f, {1,1}};
    Transform2D child{{10,0}, 0.0f, {1,1}};
    auto w = child.compose(parent);
    EXPECT_FLOAT_EQ(w.position.x, 110.0f);
    EXPECT_FLOAT_EQ(w.position.y, 100.0f);
}

TEST(Vec3, ArithmeticAndCross) {
    Vec3f a{1, 0, 0}, b{0, 1, 0};
    EXPECT_EQ(a + b, (Vec3f{1, 1, 0}));
    EXPECT_EQ(a.cross(b), (Vec3f{0, 0, 1}));
    EXPECT_FLOAT_EQ(a.dot(b), 0.0f);
    EXPECT_EQ(-a, (Vec3f{-1, 0, 0}));
    Vec3f len2{2, 0, 0};
    EXPECT_FLOAT_EQ(len2.length(), 2.0f);
    Vec3f norm{3, 4, 0};
    EXPECT_NEAR(norm.normalized().length(), 1.0f, 1e-6);
}

TEST(Vec4, SwizzleAndDot) {
    Vec4f v{1, 2, 3, 4};
    EXPECT_EQ(v.xyz(), (Vec3f{1, 2, 3}));
    EXPECT_FLOAT_EQ(v.dot(Vec4f{1,1,1,1}), 10.0f);
    EXPECT_EQ((v * 2), (Vec4f{2, 4, 6, 8}));
}

TEST(Mat4, IdentityIsIdentity) {
    Mat4 i = Mat4::identity();
    EXPECT_EQ(i, (Mat4{}));
    EXPECT_EQ(i * i, i);
    EXPECT_EQ(i(0,0), 1.0f);
    EXPECT_EQ(i(1,1), 1.0f);
    EXPECT_EQ(i(2,2), 1.0f);
    EXPECT_EQ(i(3,3), 1.0f);
}

TEST(Mat4, TranslateTransformsPoint) {
    Mat4 t = Mat4::translate(Vec3f{5, 6, 7});
    Vec3f p = t * Vec3f{1, 2, 3};
    EXPECT_FLOAT_EQ(p.x, 6.0f);
    EXPECT_FLOAT_EQ(p.y, 8.0f);
    EXPECT_FLOAT_EQ(p.z, 10.0f);
}

TEST(Mat4, RotateZ90RotatesXToY) {
    Mat4 r = Mat4::rotate_z(3.14159265358979f / 2.0f);
    Vec3f p = r * Vec3f{1, 0, 0};
    EXPECT_NEAR(p.x, 0.0f, 1e-5f);
    EXPECT_NEAR(p.y, 1.0f, 1e-5f);
}

TEST(Mat4, OrthoMapsCornersToClipSpace) {
    Mat4 p = Mat4::ortho(-1, 1, -1, 1, 0, 10);
    Vec3f tl = p * Vec3f{-1, 1, 0};
    EXPECT_NEAR(tl.x, -1.0f, 1e-5f);
    EXPECT_NEAR(tl.y, 1.0f, 1e-5f);
}

TEST(AABB, IntersectContainsMerge) {
    AABB a{0, 0, 10, 10};
    AABB b{5, 5, 10, 10};
    AABB c{20, 20, 5, 5};
    EXPECT_TRUE(a.intersects(b));
    EXPECT_FALSE(a.intersects(c));
    EXPECT_TRUE(a.contains(Vec2f{5, 5}));
    EXPECT_FALSE(a.contains(Vec2f{20, 20}));
    AABB m = a.merged(b);
    EXPECT_FLOAT_EQ(m.min.x, 0.0f);
    EXPECT_FLOAT_EQ(m.max.x, 15.0f);
    EXPECT_EQ(a.closest_point(Vec2f{-5, 5}), (Vec2f{0, 5}));
}

TEST(Rng, DeterministicReproducible) {
    Rng r1{12345};
    Rng r2{12345};
    for (int i = 0; i < 100; ++i) EXPECT_EQ(r1.next_u32(), r2.next_u32());
}

TEST(Rng, RangeBounds) {
    Rng r{99};
    for (int i = 0; i < 1000; ++i) {
        float v = r.range(-5.0f, 5.0f);
        EXPECT_GE(v, -5.0f);
        EXPECT_LT(v, 5.0f);
    }
}

TEST(Easing, Endpoints) {
    EXPECT_FLOAT_EQ(ease(Ease::Linear, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(ease(Ease::Linear, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(ease(Ease::InQuad, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(ease(Ease::OutQuad, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(ease_clamped(Ease::InCubic, -1.0f), 0.0f);
    EXPECT_FLOAT_EQ(ease_clamped(Ease::OutBounce, 2.0f), 1.0f);
    EXPECT_NEAR(ease(Ease::OutBack, 1.0f), 1.0f, 1e-5f);
}

TEST(Easing, MonotonicishOutQuad) {
    float prev = ease(Ease::OutQuad, 0.0f);
    for (int i = 1; i <= 20; ++i) {
        float t = i / 20.0f;
        float v = ease(Ease::OutQuad, t);
        EXPECT_GE(v, prev);
        prev = v;
    }
}
