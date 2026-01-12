#include <gtest/gtest.h>

#include <cmath>

#include "engine/math/constants.h"
#include "engine/render/camera3d.h"

using engine::math::Vector3f;
using engine::render::Camera3D;
using engine::render::CameraProjection;

TEST(Camera3DTest, DefaultValues) {
  Camera3D camera;

  EXPECT_FLOAT_EQ(camera.GetTarget().x, 0.0f);
  EXPECT_FLOAT_EQ(camera.GetTarget().y, 0.0f);
  EXPECT_FLOAT_EQ(camera.GetTarget().z, 0.0f);

  EXPECT_FLOAT_EQ(camera.GetDistance(), 10.0f);
  EXPECT_FLOAT_EQ(camera.GetYaw(), 0.0f);
  EXPECT_FLOAT_EQ(camera.GetPitch(), -30.0f);
  EXPECT_FLOAT_EQ(camera.GetFov(), 45.0f);
  EXPECT_EQ(camera.GetProjection(), CameraProjection::kPerspective);
}

TEST(Camera3DTest, SetTarget) {
  Camera3D camera;
  camera.SetTarget(Vector3f(5.0f, 3.0f, -2.0f));

  EXPECT_FLOAT_EQ(camera.GetTarget().x, 5.0f);
  EXPECT_FLOAT_EQ(camera.GetTarget().y, 3.0f);
  EXPECT_FLOAT_EQ(camera.GetTarget().z, -2.0f);
}

TEST(Camera3DTest, SetDistance) {
  Camera3D camera;

  camera.SetDistance(20.0f);
  EXPECT_FLOAT_EQ(camera.GetDistance(), 20.0f);

  // Test clamping to min
  camera.SetDistance(0.01f);
  EXPECT_GE(camera.GetDistance(), 0.1f);

  // Test clamping to max
  camera.SetDistance(2000.0f);
  EXPECT_LE(camera.GetDistance(), 1000.0f);
}

TEST(Camera3DTest, SetYaw) {
  Camera3D camera;
  camera.SetYaw(90.0f);
  EXPECT_FLOAT_EQ(camera.GetYaw(), 90.0f);

  camera.SetYaw(-45.0f);
  EXPECT_FLOAT_EQ(camera.GetYaw(), -45.0f);
}

TEST(Camera3DTest, SetPitch) {
  Camera3D camera;

  camera.SetPitch(45.0f);
  EXPECT_FLOAT_EQ(camera.GetPitch(), 45.0f);

  // Test clamping to max (89 degrees)
  camera.SetPitch(95.0f);
  EXPECT_LE(camera.GetPitch(), 89.0f);

  // Test clamping to min (-89 degrees)
  camera.SetPitch(-95.0f);
  EXPECT_GE(camera.GetPitch(), -89.0f);
}

TEST(Camera3DTest, OrbitHorizontal) {
  Camera3D camera;
  camera.SetYaw(0.0f);

  camera.OrbitHorizontal(45.0f);
  EXPECT_FLOAT_EQ(camera.GetYaw(), 45.0f);

  camera.OrbitHorizontal(-90.0f);
  EXPECT_FLOAT_EQ(camera.GetYaw(), -45.0f);
}

TEST(Camera3DTest, OrbitVertical) {
  Camera3D camera;
  camera.SetPitch(0.0f);

  camera.OrbitVertical(30.0f);
  EXPECT_FLOAT_EQ(camera.GetPitch(), 30.0f);

  // Pitch should be clamped
  camera.OrbitVertical(100.0f);
  EXPECT_LE(camera.GetPitch(), 89.0f);
}

TEST(Camera3DTest, Zoom) {
  Camera3D camera;
  camera.SetDistance(10.0f);

  camera.Zoom(5.0f);
  EXPECT_FLOAT_EQ(camera.GetDistance(), 15.0f);

  camera.Zoom(-10.0f);
  EXPECT_FLOAT_EQ(camera.GetDistance(), 5.0f);
}

TEST(Camera3DTest, SetFov) {
  Camera3D camera;

  camera.SetFov(60.0f);
  EXPECT_FLOAT_EQ(camera.GetFov(), 60.0f);

  // Test clamping
  camera.SetFov(0.5f);
  EXPECT_GE(camera.GetFov(), 1.0f);

  camera.SetFov(200.0f);
  EXPECT_LE(camera.GetFov(), 179.0f);
}

TEST(Camera3DTest, SetProjection) {
  Camera3D camera;

  camera.SetProjection(CameraProjection::kOrthographic);
  EXPECT_EQ(camera.GetProjection(), CameraProjection::kOrthographic);

  camera.SetProjection(CameraProjection::kPerspective);
  EXPECT_EQ(camera.GetProjection(), CameraProjection::kPerspective);
}

TEST(Camera3DTest, GetPositionFromOrbital) {
  Camera3D camera;
  camera.SetTarget(Vector3f(0.0f, 0.0f, 0.0f));
  camera.SetDistance(10.0f);
  camera.SetYaw(0.0f);
  camera.SetPitch(0.0f);

  // At yaw=0, pitch=0, camera should be on positive Z axis
  Vector3f pos = camera.GetPosition();
  EXPECT_NEAR(pos.x, 0.0f, 0.001f);
  EXPECT_NEAR(pos.y, 0.0f, 0.001f);
  EXPECT_NEAR(pos.z, 10.0f, 0.001f);
}

TEST(Camera3DTest, GetPositionWithYaw90) {
  Camera3D camera;
  camera.SetTarget(Vector3f(0.0f, 0.0f, 0.0f));
  camera.SetDistance(10.0f);
  camera.SetYaw(90.0f);
  camera.SetPitch(0.0f);

  // At yaw=90, pitch=0, camera should be on positive X axis
  Vector3f pos = camera.GetPosition();
  EXPECT_NEAR(pos.x, 10.0f, 0.001f);
  EXPECT_NEAR(pos.y, 0.0f, 0.001f);
  EXPECT_NEAR(pos.z, 0.0f, 0.001f);
}

TEST(Camera3DTest, GetPositionWithPitch) {
  Camera3D camera;
  camera.SetTarget(Vector3f(0.0f, 0.0f, 0.0f));
  camera.SetDistance(10.0f);
  camera.SetYaw(0.0f);
  camera.SetPitch(-45.0f);

  // At yaw=0, pitch=-45, camera should be elevated
  Vector3f pos = camera.GetPosition();
  EXPECT_NEAR(pos.x, 0.0f, 0.001f);
  EXPECT_GT(pos.y, 0.0f);  // Above target
  EXPECT_GT(pos.z, 0.0f);  // Still in front
}

TEST(Camera3DTest, GetUp) {
  Camera3D camera;
  Vector3f up = camera.GetUp();

  // For orbital camera, up is typically world up
  EXPECT_FLOAT_EQ(up.x, 0.0f);
  EXPECT_FLOAT_EQ(up.y, 1.0f);
  EXPECT_FLOAT_EQ(up.z, 0.0f);
}

TEST(Camera3DTest, PositionFollowsTarget) {
  Camera3D camera;
  camera.SetDistance(10.0f);
  camera.SetYaw(0.0f);
  camera.SetPitch(0.0f);

  camera.SetTarget(Vector3f(100.0f, 50.0f, -30.0f));
  Vector3f pos = camera.GetPosition();

  // Position should be offset from target
  EXPECT_NEAR(pos.x, 100.0f, 0.001f);
  EXPECT_NEAR(pos.y, 50.0f, 0.001f);
  EXPECT_NEAR(pos.z, -30.0f + 10.0f, 0.001f);
}
