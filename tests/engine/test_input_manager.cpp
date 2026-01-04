#include <gtest/gtest.h>

#include "engine/input.h"

TEST(InputManagerTest, MouseWheelDeltaIsStored) {
  engine::input::InputManager input;
  input.SetMouseWheelDelta(1.5f);
  EXPECT_FLOAT_EQ(input.GetMouseWheelDelta(), 1.5f);
}

TEST(InputManagerTest, ClearStateResetsMouseWheelDelta) {
  engine::input::InputManager input;
  input.SetMouseWheelDelta(-2.0f);
  input.ClearState();
  EXPECT_FLOAT_EQ(input.GetMouseWheelDelta(), 0.0f);
}
