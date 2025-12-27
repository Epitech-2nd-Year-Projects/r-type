#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "join_flow.h"
#include "lobby_service.h"
#include "network_transport.h"

namespace {

constexpr std::uint16_t kJoinPort = 49001;
constexpr std::uint16_t kLobbyPort = 49002;

}

TEST(JoinFlowTest, FailsWhenTransportNotRunning) {
  client::NetworkTransport transport;
  client::JoinFlow join_flow("Pilot", "room");

  join_flow.Begin(transport);

  EXPECT_EQ(join_flow.state(), client::JoinState::kRefused);
  EXPECT_EQ(join_flow.status(), "Network transport not running");
}

TEST(JoinFlowTest, TimesOutAfterRetryLimit) {
  client::NetworkTransport transport;
  const auto start_error = transport.Start("127.0.0.1", kJoinPort);
  ASSERT_FALSE(start_error);

  client::JoinFlow join_flow("Pilot", "room");
  join_flow.ConfigureRetryPolicy(1, std::chrono::milliseconds(1));
  join_flow.Begin(transport);
  ASSERT_EQ(join_flow.state(), client::JoinState::kConnecting);

  std::this_thread::sleep_for(std::chrono::milliseconds(3));
  join_flow.Update(transport);

  EXPECT_EQ(join_flow.state(), client::JoinState::kRefused);
  EXPECT_EQ(join_flow.status(), "Join timed out");
  transport.Stop();
}

TEST(LobbyServiceTest, ConnectFailsWithoutTransport) {
  client::LobbyRetryPolicy policy{};
  client::LobbyService service(nullptr, policy);

  const bool connected = service.Connect("127.0.0.1", kLobbyPort);

  EXPECT_FALSE(connected);
  EXPECT_EQ(service.status(), "Lobby transport unavailable");
}

TEST(LobbyServiceTest, RequestTimesOutWhenNoResponse) {
  auto transport = std::make_shared<client::NetworkTransport>();
  const auto start_error = transport->Start("127.0.0.1", kLobbyPort);
  ASSERT_FALSE(start_error);

  client::LobbyRetryPolicy policy{};
  policy.max_attempts = 1;
  policy.retry_delay = std::chrono::milliseconds(1);
  client::LobbyService service(transport, policy);
  ASSERT_TRUE(service.Connect("127.0.0.1", kLobbyPort));

  service.RequestRoomList();
  service.Update();
  std::this_thread::sleep_for(std::chrono::milliseconds(3));
  service.Update();

  EXPECT_EQ(service.status(), "Lobby request timed out");
  transport->Stop();
}

TEST(NetworkTransportTest, StartFailsOnInvalidHost) {
  client::NetworkTransport transport;

  const auto error = transport.Start("256.256.256.256", 1);

  EXPECT_TRUE(error);
  EXPECT_FALSE(transport.running());
  EXPECT_EQ(transport.last_receive_ms(), 0u);
}

TEST(NetworkTransportTest, LastReceiveTimestampResetsOnStop) {
  client::NetworkTransport transport;
  const auto start_error = transport.Start("127.0.0.1", kJoinPort);
  ASSERT_FALSE(start_error);

  const auto initial_ms = transport.last_receive_ms();
  ASSERT_NE(initial_ms, 0u);

  engine::net::Client::ReceivedPacket packet;
  EXPECT_FALSE(transport.Receive(packet));
  EXPECT_EQ(transport.last_receive_ms(), initial_ms);

  transport.Stop();
  EXPECT_EQ(transport.last_receive_ms(), 0u);
}
