#include <gtest/gtest.h>
#include <vector>

#include "input_buffer.h"

TEST(InputBufferTest, CapturesPressAndReleaseInOrder) {
  client::InputBuffer buffer;
  client::ActionState initial{};
  buffer.Reset(initial, 10);

  std::vector<client::GameActionEvent> events{
      {client::GameAction::kMoveUp, client::GameActionEventType::kPressed},
      {client::GameAction::kMoveUp, client::GameActionEventType::kReleased}};

  buffer.PushEvents(events, 20);

  const auto first = buffer.NextSample(30);
  EXPECT_TRUE(first.state.move_up);
  EXPECT_EQ(first.client_time_ms, 20u);

  const auto second = buffer.NextSample(40);
  EXPECT_FALSE(second.state.move_up);
  EXPECT_EQ(second.client_time_ms, 20u);
}

TEST(InputBufferTest, DeduplicatesIdenticalTransitions) {
  client::InputBuffer buffer;
  client::ActionState initial{};
  buffer.Reset(initial, 5);

  std::vector<client::GameActionEvent> events{
      {client::GameAction::kShoot, client::GameActionEventType::kPressed},
      {client::GameAction::kShoot, client::GameActionEventType::kPressed}};

  buffer.PushEvents(events, 50);

  const auto first = buffer.NextSample(60);
  EXPECT_TRUE(first.state.shoot);
  EXPECT_EQ(first.client_time_ms, 50u);

  const auto fallback = buffer.NextSample(70);
  EXPECT_TRUE(fallback.state.shoot);
  EXPECT_EQ(fallback.client_time_ms, 70u);
}

TEST(InputBufferTest, CapturesMultipleActionsPerTick) {
  client::InputBuffer buffer;
  client::ActionState initial{};
  buffer.Reset(initial, 1);

  std::vector<client::GameActionEvent> events{
      {client::GameAction::kMoveUp, client::GameActionEventType::kPressed},
      {client::GameAction::kShoot, client::GameActionEventType::kPressed}};

  buffer.PushEvents(events, 25);

  const auto first = buffer.NextSample(30);
  EXPECT_TRUE(first.state.move_up);
  EXPECT_FALSE(first.state.shoot);
  EXPECT_EQ(first.client_time_ms, 25u);

  const auto second = buffer.NextSample(40);
  EXPECT_TRUE(second.state.move_up);
  EXPECT_TRUE(second.state.shoot);
  EXPECT_EQ(second.client_time_ms, 25u);
}

TEST(InputBufferTest, FallsBackToCurrentStateWhenQueueIsEmpty) {
  client::InputBuffer buffer;
  client::ActionState initial{};
  buffer.Reset(initial, 100);

  const auto sample = buffer.NextSample(150);
  EXPECT_FALSE(sample.state.move_down);
  EXPECT_EQ(sample.client_time_ms, 150u);
}
