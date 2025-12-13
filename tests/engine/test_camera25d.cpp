#include <gtest/gtest.h>

#include "engine/render/camera25d.h"

namespace {

constexpr float kEpsilon = 0.0001f;
constexpr float kLooseEpsilon = 0.01f;

}  // namespace

TEST(Camera25DTest, MapsWorldToScreenWithParallax) {
  engine::render::Camera25D camera({800.0f, 600.0f}, 0.0f, 100.0f);
  camera.SetFocusX(50.0f);

  const auto mid = camera.WorldToScreen({50.0f, 0.0f},
                                        engine::render::RenderLayer::kMidground);
  EXPECT_NEAR(mid.x, 400.0f, kEpsilon);
  EXPECT_NEAR(mid.y, 0.0f, kEpsilon);

  const auto back = camera.WorldToScreen(
      {50.0f, 0.0f}, engine::render::RenderLayer::kBackground);
  EXPECT_NEAR(back.x, 350.0f, kLooseEpsilon);

  const auto front = camera.WorldToScreen(
      {50.0f, 0.0f}, engine::render::RenderLayer::kForeground);
  EXPECT_NEAR(front.x, 425.0f, kLooseEpsilon);
}

TEST(Camera25DTest, ComputesViewRectInWorldUnits) {
  engine::render::Camera25D camera({800.0f, 600.0f}, 0.0f, 100.0f);
  camera.SetFocusX(50.0f);

  const auto view_rect = camera.GetViewRectWorld();
  EXPECT_NEAR(view_rect.top_left_x_, -16.6666f, kLooseEpsilon);
  EXPECT_NEAR(view_rect.top_left_y_, 0.0f, kEpsilon);
  EXPECT_NEAR(view_rect.width_, 133.3333f, kLooseEpsilon);
  EXPECT_NEAR(view_rect.height_, 100.0f, kEpsilon);
}

TEST(Camera25DTest, UpdatesPixelsPerUnitWithViewportChange) {
  engine::render::Camera25D camera({800.0f, 600.0f}, 0.0f, 100.0f);
  EXPECT_NEAR(camera.GetPixelsPerUnit(), 6.0f, kEpsilon);

  camera.SetViewportSize({1920.0f, 1080.0f});
  EXPECT_NEAR(camera.GetPixelsPerUnit(), 10.8f, kEpsilon);
}

TEST(Camera25DTest, ClampsMinimumWorldHeight) {
  engine::render::Camera25D camera({800.0f, 600.0f}, 5.0f, 5.0f);
  const auto view = camera.GetViewSizeWorld();
  EXPECT_NEAR(view.y, 1.0f, kEpsilon);
}

TEST(Camera25DTest, EnforcesMinimumVerticalRangeThroughSetter) {
  engine::render::Camera25D camera({800.0f, 600.0f}, 0.0f, 10.0f);
  camera.SetVerticalRange(2.0f, 2.1f);
  EXPECT_NEAR(camera.GetVerticalMin(), 2.0f, kEpsilon);
  EXPECT_NEAR(camera.GetVerticalMax(), 3.0f, kEpsilon);
}

TEST(Camera25DTest, ClampsViewportSizeToMinimumExtent) {
  engine::render::Camera25D camera({0.5f, 0.25f}, 0.0f, 10.0f);
  EXPECT_NEAR(camera.GetViewportSize().x, 1.0f, kEpsilon);
  EXPECT_NEAR(camera.GetViewportSize().y, 1.0f, kEpsilon);
}
