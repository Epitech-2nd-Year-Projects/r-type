#ifndef ENGINE_SCRIPTING_PREFAB_FACTORY_H_
#define ENGINE_SCRIPTING_PREFAB_FACTORY_H_

#include <functional>
#include <optional>
#include <sol/sol.hpp>
#include <string>
#include <unordered_map>

#include "engine/ecs/registry.h"

namespace engine::scripting {

/**
 * @brief Factory for spawning entities defined in Lua tables (Prefabs).
 *
 * Allows registering C++ component creators that map Lua values to ECS
 * components. Entities are spawned by looking up value in the global 'Prefabs'
 * Lua table.
 */
class PrefabFactory {
 public:
  /**
   * @brief callback signature for creating a component from a Lua value.
   * @param registry The ECS registry.
   * @param entity The entity to add attributes to.
   * @param value The Lua value corresponding to the component key in the prefab
   * table.
   */
  using ComponentCreator = std::function<void(
      ecs::Registry& registry, ecs::EntityId entity, const sol::object& value)>;

  /**
   * @brief Construct with a reference to the Lua state.
   * @param lua The Lua state containing the 'Prefabs' table.
   */
  explicit PrefabFactory(sol::state& lua);

  /**
   * @brief Register a component creator function for a given key.
   * @param name The key in the Lua prefab table (e.g., "Position").
   * @param creator The function to call to add this component.
   */
  void RegisterComponent(const std::string& name, ComponentCreator creator);

  /**
   * @brief Spawn an entity based on a Lua prefab definition.
   * @param registry The registry to spawn the entity in.
   * @param prefab_name The key in the global 'Prefabs' table.
   * @return The ID of the spawned entity, or std::nullopt if failed.
   */
  std::optional<ecs::EntityId> Spawn(ecs::Registry& registry,
                                     const std::string& prefab_name);

 private:
  sol::state& lua_;
  std::unordered_map<std::string, ComponentCreator> creators_;
};

}  // namespace engine::scripting

#endif  // ENGINE_SCRIPTING_PREFAB_FACTORY_H_
