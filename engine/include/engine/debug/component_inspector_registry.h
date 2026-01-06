#ifndef ENGINE_DEBUG_COMPONENT_INSPECTOR_REGISTRY_H_
#define ENGINE_DEBUG_COMPONENT_INSPECTOR_REGISTRY_H_

#include <any>
#include <functional>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "engine/ecs/entity_id.h"
#include "engine/ecs/registry.h"

namespace engine::debug {

/**
 * @brief Draw callback for component inspection
 *
 * @param registry Reference to the main ECS registry
 * @param entity_id The entity to inspect
 * @return true if the component was modified
 */
using InspectorDrawCallback =
    std::function<bool(ecs::Registry&, const ecs::EntityId&)>;

/**
 * @struct ComponentMeta
 * @brief Metadata for a registered component inspector
 */
struct ComponentMeta {
  std::string display_name;       ///< Human-readable name for ImGui
  InspectorDrawCallback draw_fn;  ///< Type-erased draw function
};

/**
 * @class ComponentInspectorRegistry
 * @brief Registry for component inspectors using type erasure
 *
 * @details
 * This registry allows game code to register ImGui inspectors for
 * their components without the engine needing to know about them.
 * Uses std::any with std::reference_wrapper<T> to avoid raw pointers.
 *
 */
class ComponentInspectorRegistry {
 public:
  ComponentInspectorRegistry() = default;
  ~ComponentInspectorRegistry() = default;

  ComponentInspectorRegistry(const ComponentInspectorRegistry&) = default;
  ComponentInspectorRegistry& operator=(const ComponentInspectorRegistry&) =
      default;
  ComponentInspectorRegistry(ComponentInspectorRegistry&&) noexcept = default;
  ComponentInspectorRegistry& operator=(ComponentInspectorRegistry&&) noexcept =
      default;

  /**
   * @brief Register an inspector for a component type
   *
   * @tparam T The component type to register
   * @param display_name Human-readable name for the inspector panel
   * @param callback Function to draw the inspector UI
   *
   * @details
   * The callback receives a mutable reference to the component and
   * the entity ID. It should return true if the component was modified.
   */
  template <typename T>
  void Register(std::string display_name,
                std::function<bool(T&, const ecs::EntityId&)> callback) {
    auto type_erased = [cb = std::move(callback)](
                           ecs::Registry& registry,
                           const ecs::EntityId& entity_id) -> bool {
      try {
        auto& components = registry.GetComponents<T>();
        auto idx = static_cast<std::size_t>(entity_id);

        if (idx < components.size() && components[idx].has_value()) {
          return cb(components[idx].value(), entity_id);
        }
      } catch (const std::exception&) {
      }
      return false;
    };

    inspectors_[std::type_index(typeid(T))] =
        ComponentMeta{std::move(display_name), std::move(type_erased)};
  }

  /**
   * @brief Lookup an inspector by component type
   *
   * @param type The type_index of the component
   * @return Optional reference to the ComponentMeta, or nullopt if not found
   */
  std::optional<std::reference_wrapper<const ComponentMeta>> Get(
      std::type_index type) const;

  /**
   * @brief Check if an inspector is registered for a type
   *
   * @param type The type_index to check
   * @return true if an inspector is registered
   */
  bool HasInspector(std::type_index type) const;

  /**
   * @brief Get all registered inspectors
   *
   * @return Const reference to the internal map
   */
  const std::unordered_map<std::type_index, ComponentMeta>& All() const;

  /**
   * @brief Get the number of registered inspectors
   *
   * @return Number of registered component types
   */
  std::size_t Size() const noexcept;

 private:
  std::unordered_map<std::type_index, ComponentMeta> inspectors_;
};

}  // namespace engine::debug

#endif  // ENGINE_DEBUG_COMPONENT_INSPECTOR_REGISTRY_H_
