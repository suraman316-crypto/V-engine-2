#include <vengine/math/Color.hpp>
#include <vengine/math/Rect.hpp>
#include <vengine/math/Transform2D.hpp>
#include <vengine/math/Vec2.hpp>

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
