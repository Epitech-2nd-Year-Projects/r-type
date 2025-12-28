#include <gtest/gtest.h>

#include "game_logic/game_config.h"

TEST(GameConfigTest, LoadConfigs) {
  bool success = game_logic::GameConfig::Get().Load("config");
  ASSERT_TRUE(success)
      << "Failed to load configuration from 'config' directory";
}

TEST(GameConfigTest, VerifyGlobalConfig) {
  const auto& w = game_logic::GameConfig::Get().GetWorld();
  EXPECT_EQ(w.grid_cell_size, 100.0f);
  EXPECT_EQ(w.spawn_max_y, 700.0f);
  EXPECT_EQ(w.player_spawn_y, 300.0f);
}

TEST(GameConfigTest, VerifyLevelConfig) {
  ASSERT_NO_THROW({
    const auto& level1 = game_logic::GameConfig::Get().GetLevel(1);
    EXPECT_EQ(level1.id, 1);
    EXPECT_FALSE(level1.waves.empty());
    EXPECT_EQ(level1.waves[0].time, 2.0f);
  });
}
