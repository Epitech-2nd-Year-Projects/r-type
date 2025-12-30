#include "game_logic/systems/wave_system.h"

#include <filesystem>
#include <vector>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/scripting/script_engine.h"
#include "engine/util/logging.h"
#include "game_logic/components/powerup_drop_component.h"
#include "game_logic/constants.h"
#include "game_logic/game_config.h"
#include "game_logic/game_instance.h"

namespace game_logic::systems {

WaveSystem::WaveSystem(GameInstance &game_instance)
    : game_instance_(game_instance), rng_(std::random_device{}()) {
  auto &logger = engine::util::Logger::Default();
  try {
    std::string config_dir = GameConfig::Get().GetConfigDirectory();
    sol::state &lua = game_instance_.ScriptEngine().LuaState();

    game_instance_.ScriptEngine().LoadScript(config_dir +
                                             "/levels/level_manager.lua");

    sol::function init_func = lua["LevelManager"]["Init"];
    if (init_func.valid()) {
      init_func(config_dir);
    }

    sol::function load_func = lua["LevelManager"]["LoadLevel"];
    if (load_func.valid()) {
      load_func(1);
    }

  } catch (const std::exception &e) {
    logger.Error("[WaveSystem] Failed to init level manager: ", e.what());
  }
}

void WaveSystem::LoadLevel(int level_id) {
  sol::state &lua = game_instance_.ScriptEngine().LuaState();
  sol::function load_func = lua["LevelManager"]["LoadLevel"];
  if (load_func.valid()) {
    load_func(level_id);
  }
}

void WaveSystem::Update(engine::ecs::Registry &registry,
                        engine::time::TimeDelta dt) {
  sol::state &lua = game_instance_.ScriptEngine().LuaState();

  bool enemies_alive = false;
  auto &tags = registry.GetComponents<engine::ecs::TagComponent>();
  for (const auto &tag : tags) {
    if (tag.has_value() && tag->tag == "Enemy") {
      enemies_alive = true;
      break;
    }
  }

  sol::function update_func = lua["LevelManager"]["Update"];
  if (update_func.valid()) {
    update_func(dt.as_seconds(), enemies_alive);
  }

  sol::optional<int> current_id = lua["LevelManager"]["current_id"];
  if (current_id) {
    auto &state = game_instance_.State();
    state.current_level = static_cast<std::uint32_t>(current_id.value());
    state.current_wave = static_cast<std::uint32_t>(current_id.value());
  }
}

}  // namespace game_logic::systems
