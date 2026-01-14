#include <gtest/gtest.h>

#include "ecs/components.h"
#include "engine/ecs/registry.h"
#include "hud_overlay.h"

TEST(HudOverlayTest, BuildsPlayerRowsFromRegistry) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<client::ecs::NetworkedEntityComponent>();
  registry.RegisterComponent<client::ecs::HealthComponent>();
  registry.RegisterComponent<client::ecs::PlayerStateComponent>();

  auto first = registry.SpawnEntity();
  auto second = registry.SpawnEntity();

  registry.EmplaceComponent<client::ecs::NetworkedEntityComponent>(first, 5u,
                                                                   1u, 1u);
  registry.EmplaceComponent<client::ecs::NetworkedEntityComponent>(second, 7u,
                                                                   1u, 1u);
  registry.EmplaceComponent<client::ecs::HealthComponent>(first, 3u, 3u);
  registry.EmplaceComponent<client::ecs::HealthComponent>(second, 0u, 3u);
  registry.EmplaceComponent<client::ecs::PlayerStateComponent>(first, 5u, 120u,
                                                               3u);
  registry.EmplaceComponent<client::ecs::PlayerStateComponent>(second, 7u, 0u,
                                                               0u);

  client::HudOverlay overlay;
  overlay.UpdatePlayers(registry, 5u);

  const auto& players = overlay.players();
  ASSERT_EQ(players.size(), 2u);
  EXPECT_EQ(players[0].player_id, 5u);
  EXPECT_TRUE(players[0].is_local);
  EXPECT_TRUE(players[0].hp.has_value());
  EXPECT_TRUE(players[0].max_hp.has_value());
  EXPECT_TRUE(players[0].lives.has_value());
  EXPECT_TRUE(players[0].score.has_value());
  EXPECT_EQ(players[0].hp.value(), 3u);
  EXPECT_EQ(players[0].max_hp.value(), 3u);
  EXPECT_EQ(players[0].lives.value(), 3u);
  EXPECT_EQ(players[0].score.value(), 120u);
  EXPECT_TRUE(players[0].alive);

  EXPECT_EQ(players[1].player_id, 7u);
  EXPECT_FALSE(players[1].is_local);
  EXPECT_TRUE(players[1].lives.has_value());
  EXPECT_EQ(players[1].lives.value(), 0u);
  EXPECT_TRUE(players[1].score.has_value());
  EXPECT_EQ(players[1].score.value(), 0u);
  EXPECT_TRUE(players[1].hp.has_value());
  EXPECT_TRUE(players[1].max_hp.has_value());
  EXPECT_EQ(players[1].hp.value(), 0u);
  EXPECT_EQ(players[1].max_hp.value(), 3u);
  EXPECT_FALSE(players[1].alive);
}

TEST(HudOverlayTest, TracksNetworkStateAndLatency) {
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

TEST(HudOverlayTest, UpdatesWaveAndLevel) {
  client::HudOverlay overlay;
  overlay.UpdateWave(std::nullopt);
  EXPECT_FALSE(overlay.wave().has_value());
}

TEST(HudOverlayTest, AliveFlagReflectsHealthChanges) {
  engine::ecs::Registry registry;
  registry.RegisterComponent<client::ecs::NetworkedEntityComponent>();
  registry.RegisterComponent<client::ecs::HealthComponent>();

  registry.RegisterComponent<client::ecs::PlayerStateComponent>();
  auto entity = registry.SpawnEntity();
  registry.EmplaceComponent<client::ecs::NetworkedEntityComponent>(entity, 42u,
                                                                   1u, 1u);
  registry.EmplaceComponent<client::ecs::HealthComponent>(entity, 1u, 1u);

  client::HudOverlay overlay;
  overlay.UpdatePlayers(registry, std::nullopt);
  ASSERT_EQ(overlay.players().size(), 1u);
  EXPECT_TRUE(overlay.players()[0].alive);

  registry.GetComponents<client::ecs::HealthComponent>()[entity]->current = 0;
  overlay.UpdatePlayers(registry, std::nullopt);
  ASSERT_EQ(overlay.players().size(), 1u);
  EXPECT_FALSE(overlay.players()[0].alive);
}
