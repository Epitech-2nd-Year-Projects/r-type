#include <gtest/gtest.h>

#include <cmath>

#include "engine/math/vector3.h"

using engine::math::Vector3f;

TEST(Vector3Test, DefaultConstructor) {
  Vector3f v;
  EXPECT_FLOAT_EQ(v.x, 0.0f);
  EXPECT_FLOAT_EQ(v.y, 0.0f);
  EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST(Vector3Test, ParameterizedConstructor) {
  Vector3f v(1.0f, 2.0f, 3.0f);
  EXPECT_FLOAT_EQ(v.x, 1.0f);
  EXPECT_FLOAT_EQ(v.y, 2.0f);
  EXPECT_FLOAT_EQ(v.z, 3.0f);
}

TEST(Vector3Test, Addition) {
  Vector3f a(1.0f, 2.0f, 3.0f);
  Vector3f b(4.0f, 5.0f, 6.0f);
  Vector3f result = a + b;
  EXPECT_FLOAT_EQ(result.x, 5.0f);
  EXPECT_FLOAT_EQ(result.y, 7.0f);
  EXPECT_FLOAT_EQ(result.z, 9.0f);
}

TEST(Vector3Test, Subtraction) {
  Vector3f a(4.0f, 5.0f, 6.0f);
  Vector3f b(1.0f, 2.0f, 3.0f);
  Vector3f result = a - b;
  EXPECT_FLOAT_EQ(result.x, 3.0f);
  EXPECT_FLOAT_EQ(result.y, 3.0f);
  EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST(Vector3Test, ScalarMultiplication) {
  Vector3f v(1.0f, 2.0f, 3.0f);
  Vector3f result = v * 2.0f;
  EXPECT_FLOAT_EQ(result.x, 2.0f);
  EXPECT_FLOAT_EQ(result.y, 4.0f);
  EXPECT_FLOAT_EQ(result.z, 6.0f);
}

TEST(Vector3Test, ScalarDivision) {
  Vector3f v(2.0f, 4.0f, 6.0f);
  Vector3f result = v / 2.0f;
  EXPECT_FLOAT_EQ(result.x, 1.0f);
  EXPECT_FLOAT_EQ(result.y, 2.0f);
  EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST(Vector3Test, Negation) {
  Vector3f v(1.0f, -2.0f, 3.0f);
  Vector3f result = -v;
  EXPECT_FLOAT_EQ(result.x, -1.0f);
  EXPECT_FLOAT_EQ(result.y, 2.0f);
  EXPECT_FLOAT_EQ(result.z, -3.0f);
}

TEST(Vector3Test, DotProduct) {
  Vector3f a(1.0f, 2.0f, 3.0f);
  Vector3f b(4.0f, 5.0f, 6.0f);
  float dot = a.Dot(b);
  EXPECT_FLOAT_EQ(dot, 32.0f);  // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
}

TEST(Vector3Test, CrossProduct) {
  Vector3f a(1.0f, 0.0f, 0.0f);  // X axis
  Vector3f b(0.0f, 1.0f, 0.0f);  // Y axis
  Vector3f result = a.Cross(b);
  EXPECT_FLOAT_EQ(result.x, 0.0f);
  EXPECT_FLOAT_EQ(result.y, 0.0f);
  EXPECT_FLOAT_EQ(result.z, 1.0f);  // Z axis
}

TEST(Vector3Test, Length) {
  Vector3f v(3.0f, 4.0f, 0.0f);
  EXPECT_FLOAT_EQ(v.Length(), 5.0f);
}

TEST(Vector3Test, LengthSquared) {
  Vector3f v(3.0f, 4.0f, 0.0f);
  EXPECT_FLOAT_EQ(v.LengthSquared(), 25.0f);
}

TEST(Vector3Test, Distance) {
  Vector3f a(0.0f, 0.0f, 0.0f);
  Vector3f b(3.0f, 4.0f, 0.0f);
  EXPECT_FLOAT_EQ(a.Distance(b), 5.0f);
}

TEST(Vector3Test, Normalized) {
  Vector3f v(3.0f, 0.0f, 4.0f);
  Vector3f normalized = v.Normalized();
  EXPECT_FLOAT_EQ(normalized.Length(), 1.0f);
  EXPECT_FLOAT_EQ(normalized.x, 0.6f);
  EXPECT_FLOAT_EQ(normalized.y, 0.0f);
  EXPECT_FLOAT_EQ(normalized.z, 0.8f);
}

TEST(Vector3Test, NormalizedZeroVector) {
  Vector3f v(0.0f, 0.0f, 0.0f);
  Vector3f normalized = v.Normalized();
  EXPECT_FLOAT_EQ(normalized.x, 0.0f);
  EXPECT_FLOAT_EQ(normalized.y, 0.0f);
  EXPECT_FLOAT_EQ(normalized.z, 0.0f);
}

TEST(Vector3Test, Lerp) {
  Vector3f a(0.0f, 0.0f, 0.0f);
  Vector3f b(10.0f, 20.0f, 30.0f);

  Vector3f midpoint = Vector3f::Lerp(a, b, 0.5f);
  EXPECT_FLOAT_EQ(midpoint.x, 5.0f);
  EXPECT_FLOAT_EQ(midpoint.y, 10.0f);
  EXPECT_FLOAT_EQ(midpoint.z, 15.0f);

  Vector3f start = Vector3f::Lerp(a, b, 0.0f);
  EXPECT_FLOAT_EQ(start.x, 0.0f);
  EXPECT_FLOAT_EQ(start.y, 0.0f);
  EXPECT_FLOAT_EQ(start.z, 0.0f);

  Vector3f end = Vector3f::Lerp(a, b, 1.0f);
  EXPECT_FLOAT_EQ(end.x, 10.0f);
  EXPECT_FLOAT_EQ(end.y, 20.0f);
  EXPECT_FLOAT_EQ(end.z, 30.0f);
}

TEST(Vector3Test, StaticDirections) {
  Vector3f up = Vector3f::Up();
  EXPECT_FLOAT_EQ(up.x, 0.0f);
  EXPECT_FLOAT_EQ(up.y, 1.0f);
  EXPECT_FLOAT_EQ(up.z, 0.0f);

  Vector3f forward = Vector3f::Forward();
  EXPECT_FLOAT_EQ(forward.x, 0.0f);
  EXPECT_FLOAT_EQ(forward.y, 0.0f);
  EXPECT_FLOAT_EQ(forward.z, -1.0f);

  Vector3f right = Vector3f::Right();
  EXPECT_FLOAT_EQ(right.x, 1.0f);
  EXPECT_FLOAT_EQ(right.y, 0.0f);
  EXPECT_FLOAT_EQ(right.z, 0.0f);

  Vector3f zero = Vector3f::Zero();
  EXPECT_FLOAT_EQ(zero.x, 0.0f);
  EXPECT_FLOAT_EQ(zero.y, 0.0f);
  EXPECT_FLOAT_EQ(zero.z, 0.0f);

  Vector3f one = Vector3f::One();
  EXPECT_FLOAT_EQ(one.x, 1.0f);
  EXPECT_FLOAT_EQ(one.y, 1.0f);
  EXPECT_FLOAT_EQ(one.z, 1.0f);
}

TEST(Vector3Test, Equality) {
  Vector3f a(1.0f, 2.0f, 3.0f);
  Vector3f b(1.0f, 2.0f, 3.0f);
  Vector3f c(1.0f, 2.0f, 4.0f);

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
  EXPECT_FALSE(a != b);
  EXPECT_TRUE(a != c);
}

TEST(Vector3Test, CompoundAssignment) {
  Vector3f v(1.0f, 2.0f, 3.0f);

  v += Vector3f(1.0f, 1.0f, 1.0f);
  EXPECT_FLOAT_EQ(v.x, 2.0f);
  EXPECT_FLOAT_EQ(v.y, 3.0f);
  EXPECT_FLOAT_EQ(v.z, 4.0f);

  v -= Vector3f(1.0f, 1.0f, 1.0f);
  EXPECT_FLOAT_EQ(v.x, 1.0f);
  EXPECT_FLOAT_EQ(v.y, 2.0f);
  EXPECT_FLOAT_EQ(v.z, 3.0f);

  v *= 2.0f;
  EXPECT_FLOAT_EQ(v.x, 2.0f);
  EXPECT_FLOAT_EQ(v.y, 4.0f);
  EXPECT_FLOAT_EQ(v.z, 6.0f);

  v /= 2.0f;
  EXPECT_FLOAT_EQ(v.x, 1.0f);
  EXPECT_FLOAT_EQ(v.y, 2.0f);
  EXPECT_FLOAT_EQ(v.z, 3.0f);
}
