#include "hud_overlay.h"

#include <gtest/gtest.h>

#include "ecs/components.h"
#include "engine/ecs/registry.h"

TEST(HudOverlayTests, BuildsPlayerRowsFromRegistry) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<client::ecs::NetworkedEntityComponent>();
  registry.RegisterComponent<client::ecs::HealthComponent>();

  auto first = registry.SpawnEntity();
  auto second = registry.SpawnEntity();

  registry.EmplaceComponent<client::ecs::NetworkedEntityComponent>(
      first, 5u, 1u, 1u);
  registry.EmplaceComponent<client::ecs::NetworkedEntityComponent>(
      second, 7u, 1u, 1u);
  registry.EmplaceComponent<client::ecs::HealthComponent>(first, 3u, 3u);
  registry.EmplaceComponent<client::ecs::HealthComponent>(second, 0u, 3u);

  client::HudOverlay overlay;
  overlay.UpdatePlayers(registry, 5u);

  const auto& players = overlay.players();
  ASSERT_EQ(players.size(), 2u);
  EXPECT_EQ(players[0].player_id, 5u);
  EXPECT_TRUE(players[0].is_local);
  EXPECT_EQ(players[0].lives, 3u);
  EXPECT_TRUE(players[0].alive);

  EXPECT_EQ(players[1].player_id, 7u);
  EXPECT_FALSE(players[1].is_local);
  EXPECT_EQ(players[1].lives, 0u);
  EXPECT_FALSE(players[1].alive);
}

TEST(HudOverlayTests, TracksNetworkStateAndLatency) {
  client::HudOverlay overlay;
  overlay.UpdateNetwork(42.5f, true, "Connected");

  ASSERT_TRUE(overlay.latency_ms().has_value());
  EXPECT_NEAR(overlay.latency_ms().value(), 42.5f, 0.01f);
  EXPECT_TRUE(overlay.connected());
  EXPECT_EQ(overlay.status_text(), "Connected");

  overlay.UpdateNetwork(std::nullopt, false, "");
  EXPECT_FALSE(overlay.latency_ms().has_value());
  EXPECT_FALSE(overlay.connected());
  EXPECT_EQ(overlay.status_text(), "Disconnected");
}
