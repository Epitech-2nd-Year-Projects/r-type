#include "debug_overlay.h"
#include "world_update_receiver.h"

#include <gtest/gtest.h>
#include <optional>

#include "engine/time/time_delta.h"
#include "protocol/ping.h"

class DebugOverlayTestPeer {
 public:
  static float AverageMs(const client::DebugOverlay& overlay) {
    return overlay.AverageFrameTimeMs();
  }

  static std::optional<float> Latency(const client::DebugOverlay& overlay) {
    return overlay.last_latency_ms_;
  }

  static std::size_t Entities(const client::DebugOverlay& overlay) {
    return overlay.last_entity_count_;
  }

  static bool Enabled(const client::DebugOverlay& overlay) {
    return overlay.enabled_;
  }
};

class WorldUpdateReceiverTestPeer {
 public:
  static void SimulatePong(client::WorldUpdateReceiver& receiver,
                           std::uint32_t ping_time, std::uint32_t server_time,
                           std::uint32_t now) {
    receiver.latency_estimator_.OnPingSent(ping_time);
    protocol::PongPayload pong{};
    pong.client_time_ms = ping_time;
    pong.server_time_ms = server_time;
    receiver.HandlePong(pong, now);
  }
};

TEST(DebugOverlayTests, ToggleAndStateUpdates) {
  client::DebugOverlay overlay;
  EXPECT_FALSE(DebugOverlayTestPeer::Enabled(overlay));
  overlay.Toggle();
  EXPECT_TRUE(DebugOverlayTestPeer::Enabled(overlay));
  overlay.Toggle();
  EXPECT_FALSE(DebugOverlayTestPeer::Enabled(overlay));

  overlay.UpdateRenderableCount(42);
  EXPECT_EQ(DebugOverlayTestPeer::Entities(overlay), 42u);

  overlay.UpdateLatency(12.5f);
  ASSERT_TRUE(DebugOverlayTestPeer::Latency(overlay).has_value());
  EXPECT_FLOAT_EQ(DebugOverlayTestPeer::Latency(overlay).value(), 12.5f);
}

TEST(DebugOverlayTests, ComputesAverageFrameTime) {
  client::DebugOverlay overlay;
  overlay.UpdateFrameTiming(engine::time::TimeDelta::from_milliseconds(10.0f));
  overlay.UpdateFrameTiming(engine::time::TimeDelta::from_milliseconds(20.0f));

  EXPECT_NEAR(DebugOverlayTestPeer::AverageMs(overlay), 15.0f, 0.01f);
}

TEST(WorldUpdateReceiverTests, UpdatesLatencyFromPong) {
  client::WorldUpdateReceiver receiver;

  EXPECT_FALSE(receiver.LatestRttMs().has_value());

  WorldUpdateReceiverTestPeer::SimulatePong(receiver, 100u, 150u, 220u);

  ASSERT_TRUE(receiver.LatestRttMs().has_value());
  EXPECT_NEAR(receiver.LatestRttMs().value(), 120.0f, 0.1f);
}
#include <optional>
