#include <gtest/gtest.h>

#include "engine/input.h"
#include "engine/time/time_delta.h"
#include "engine/ui/text_input.h"

TEST(TextInputTest, IgnoresTypingWhenNotFocused) {
  engine::ui::TextInput input_field({0.0f, 0.0f}, {200.0f, 40.0f});
  engine::input::InputManager input;

  input.HandleKey(engine::input::Key::kA, true);
  input_field.Update(engine::time::TimeDelta::from_seconds(0.01f), input);

  EXPECT_TRUE(input_field.GetText().empty());

  input_field.SetFocused(true);
  input_field.Update(engine::time::TimeDelta::from_seconds(0.01f), input);

  EXPECT_EQ(input_field.GetText(), "a");

  input.HandleKey(engine::input::Key::kA, false);
  input_field.Update(engine::time::TimeDelta::from_seconds(0.01f), input);
}

TEST(TextInputTest, BackspaceRemovesSingleCharacter) {
  engine::ui::TextInput input_field({0.0f, 0.0f}, {200.0f, 40.0f});
  engine::input::InputManager input;

  input_field.SetFocused(true);
  input_field.SetText("ab");

  input.HandleKey(engine::input::Key::kBackspace, true);
  input_field.Update(engine::time::TimeDelta::from_seconds(0.01f), input);

  EXPECT_EQ(input_field.GetText(), "a");

  input.HandleKey(engine::input::Key::kBackspace, false);
  input_field.Update(engine::time::TimeDelta::from_seconds(0.01f), input);
}
