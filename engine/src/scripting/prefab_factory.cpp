#include "engine/scripting/prefab_factory.h"

#include "engine/util/logging.h"

namespace engine::scripting {

PrefabFactory::PrefabFactory(sol::state& lua) : lua_(lua) {}

void PrefabFactory::RegisterComponent(const std::string& name,
                                      ComponentCreator creator) {
  creators_[name] = std::move(creator);
}

std::optional<ecs::EntityId> PrefabFactory::Spawn(
    ecs::Registry& registry, const std::string& prefab_name) {
  sol::optional<sol::table> prefabs_opt = lua_["Prefabs"];
  if (!prefabs_opt) {
    ENGINE_LOG_ERROR("Lua global 'Prefabs' table not found.");
    return std::nullopt;
  }

  sol::table prefabs = prefabs_opt.value();
  sol::optional<sol::table> entity_def_opt = prefabs[prefab_name];

  if (!entity_def_opt) {
    ENGINE_LOG_ERROR("Prefab '{}' not found in Lua 'Prefabs' table.",
                     prefab_name);
    return std::nullopt;
  }

  sol::table entity_def = entity_def_opt.value();
  ecs::EntityId entity = registry.SpawnEntity();

  for (const auto& kv : entity_def) {
    if (!kv.first.is<std::string>()) {
      continue;
    }

    std::string key = kv.first.as<std::string>();
    sol::object value = kv.second;

    auto it = creators_.find(key);
    if (it != creators_.end()) {
      try {
        it->second(registry, entity, value);
      } catch (const std::exception& e) {
        ENGINE_LOG_ERROR("Error creating component '{}' for prefab '{}': {}",
                         key, prefab_name, e.what());
      }
    } else {
      ENGINE_LOG_WARN("Unknown component key '{}' in prefab '{}'", key,
                      prefab_name);
    }
  }

  return entity;
}

}  // namespace engine::scripting
