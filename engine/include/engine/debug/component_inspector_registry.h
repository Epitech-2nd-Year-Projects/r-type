#ifndef ENGINE_DEBUG_COMPONENT_INSPECTOR_REGISTRY_H_
#define ENGINE_DEBUG_COMPONENT_INSPECTOR_REGISTRY_H_

#include <any>
#include <functional>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "engine/ecs/entity_id.h"

namespace engine::debug {

/**
 * @brief Type-erased draw callback for component inspection
 *
 * @details
 * The std::any parameter contains a std::reference_wrapper<T> to the
 * component. This design avoids raw pointers entirely.
 *
 * @param component_any std::any containing reference_wrapper to the component
 * @param entity_id The entity owning this component
 * @return true if the component was modified by the inspector
 */
using InspectorDrawCallback =
    std::function<bool(std::any&, const ecs::EntityId&)>;

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
                           std::any& data,
                           const ecs::EntityId& entity_id) -> bool {
      auto& ref_wrapper = std::any_cast<std::reference_wrapper<T>&>(data);
      return cb(ref_wrapper.get(), entity_id);
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
