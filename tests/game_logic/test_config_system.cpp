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
  EXPECT_EQ(w.player_spawn_y, 300.0f);
}

#include "engine/scripting/script_engine.h"

TEST(GameConfigTest, LoadDifficultyConfig) {
  engine::scripting::ScriptEngine script_engine;
  script_engine.Initialize();
  auto& lua = script_engine.LuaState();

  std::string config_path =
      game_logic::GameConfig::Get().GetConfigDirectory() + "/difficulty.lua";
  auto result = lua.safe_script_file(config_path);
  ASSERT_TRUE(result.valid()) << "Failed to load " << config_path << ": "
                              << result.get<sol::error>().what();

  sol::table settings = lua["DifficultySettings"];
  ASSERT_TRUE(settings.valid()) << "DifficultySettings table not found";

  sol::table hard = settings["Hard"];
  ASSERT_TRUE(hard.valid()) << "Difficulty 'Hard' not defined";

  float speed_mult = hard["enemy_speed_multiplier"].get_or(0.0f);
  EXPECT_GT(speed_mult, 1.0f);
  EXPECT_NEAR(speed_mult, 1.3f, 0.01f);

  int lives = hard["player_lives"].get_or(0);
  EXPECT_EQ(lives, 2);

  sol::table hardcore = settings["Hardcore"];
  ASSERT_TRUE(hardcore.valid()) << "Difficulty 'Hardcore' not defined";

  int hp = hardcore["player_health"].get_or(0);
  EXPECT_EQ(hp, 1);
}
