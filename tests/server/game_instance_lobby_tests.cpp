#include <gtest/gtest.h>

#include "game_instance.h"
#include "protocol/command.h"

namespace {

protocol::CommandPayload MakeCommand(protocol::CommandType type) {
  protocol::CommandPayload cmd{};
  cmd.command_id = static_cast<std::uint16_t>(type);
  return cmd;
}

}  // namespace

TEST(GameInstanceLobbyTests, UnknownPlayerCommandIgnored) {
  auto& logger = engine::util::Logger::Default();
  server::GameInstance instance{1, 1234, 4, protocol::Difficulty::kNormal,
                                logger};

  const auto evt = instance.OnClientCommand(
      42, MakeCommand(protocol::CommandType::kSetReady));
  EXPECT_FALSE(evt.has_value());
}

TEST(GameInstanceLobbyTests, ReadyToggleSinglePlayerNoStart) {
  auto& logger = engine::util::Logger::Default();
  server::GameInstance instance{2, 9999, 4, protocol::Difficulty::kNormal,
                                logger};

  instance.OnPlayerJoined(1, "p1");

  auto evt_ready = instance.OnClientCommand(
      1, MakeCommand(protocol::CommandType::kSetReady));
  ASSERT_TRUE(evt_ready.has_value());
  EXPECT_TRUE(evt_ready->is_ready);
  EXPECT_FALSE(evt_ready->game_started);

  auto evt_duplicate = instance.OnClientCommand(
      1, MakeCommand(protocol::CommandType::kSetReady));
  EXPECT_FALSE(evt_duplicate.has_value());

  auto evt_unready =
      instance.OnClientCommand(1, MakeCommand(protocol::CommandType::kUnready));
  ASSERT_TRUE(evt_unready.has_value());
  EXPECT_FALSE(evt_unready->is_ready);
  EXPECT_FALSE(evt_unready->game_started);
}

TEST(GameInstanceLobbyTests, StartWhenTwoPlayersReady) {
  auto& logger = engine::util::Logger::Default();
  server::GameInstance instance{3, 2024, 4, protocol::Difficulty::kNormal,
                                logger};

  instance.OnPlayerJoined(1, "p1");
  instance.OnPlayerJoined(2, "p2");

  auto evt_p1 = instance.OnClientCommand(
      1, MakeCommand(protocol::CommandType::kSetReady));
  ASSERT_TRUE(evt_p1.has_value());
  EXPECT_FALSE(evt_p1->game_started);

  auto evt_p2 = instance.OnClientCommand(
      2, MakeCommand(protocol::CommandType::kSetReady));
  ASSERT_TRUE(evt_p2.has_value());
  EXPECT_TRUE(evt_p2->game_started);
}

TEST(GameInstanceLobbyTests, ResetToLobbyAfterAllLeave) {
  auto& logger = engine::util::Logger::Default();
  server::GameInstance instance{4, 5555, 4, protocol::Difficulty::kNormal,
                                logger};

  instance.OnPlayerJoined(1, "p1");
  instance.OnPlayerJoined(2, "p2");
  instance.OnClientCommand(1, MakeCommand(protocol::CommandType::kSetReady));
  instance.OnClientCommand(2, MakeCommand(protocol::CommandType::kSetReady));

  instance.OnPlayerLeft(1);
  instance.OnPlayerLeft(2);

  instance.OnPlayerJoined(3, "p3");
  instance.OnPlayerJoined(4, "p4");

  auto evt_p3 = instance.OnClientCommand(
      3, MakeCommand(protocol::CommandType::kSetReady));
  ASSERT_TRUE(evt_p3.has_value());
  EXPECT_FALSE(evt_p3->game_started);

  auto evt_p4 = instance.OnClientCommand(
      4, MakeCommand(protocol::CommandType::kSetReady));
  ASSERT_TRUE(evt_p4.has_value());
  EXPECT_TRUE(evt_p4->game_started);
}
