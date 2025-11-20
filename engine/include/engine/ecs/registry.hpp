#ifndef ENGINE_ECS_REGISTRY_H_
#define ENGINE_ECS_REGISTRY_H_

#include <any>
#include <functional>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "entity_id.hpp"
#include "sparse_array.hpp"

namespace engine::ecs {

class Registry {
 public:
  template <class Component>
  SparseArray<Component>& RegisterComponent() {
    auto type_idx = std::type_index(typeid(Component));

    if (components_arrays_.find(type_idx) == components_arrays_.end()) {
      components_arrays_[type_idx] = SparseArray<Component>();
      component_deleters_.push_back([](Registry& reg, const Entity& e) {
        auto& components = reg.GetComponents<Component>();
        components.Erase(static_cast<std::size_t>(e));
      });
    }
    return std::any_cast<SparseArray<Component>&>(components_arrays_[type_idx]);
  }

  template <class Component>
  SparseArray<Component>& GetComponents() {
    auto type_idx = std::type_index(typeid(Component));
    auto it = components_arrays_.find(type_idx);

    if (it == components_arrays_.end())
      throw std::runtime_error("Component type not registered");
    return std::any_cast<SparseArray<Component>&>(components_arrays_[type_idx]);
  }

  template <class Component>
  const SparseArray<Component>& GetComponents() const {
    auto type_idx = std::type_index(typeid(Component));
    auto it = components_arrays_.find(type_idx);

    if (it == components_arrays_.end())
      throw std::runtime_error("Component type not registered");
    return std::any_cast<const SparseArray<Component>&>(it->second);
  }

  Entity SpawnEntity() noexcept { return Entity(next_entity_id_++); }

  Entity EntityFromIndex(std::size_t idx) noexcept { return Entity(idx); }

  void KillEntity(const Entity& e) {
    for (auto& deleter : component_deleters_) {
      deleter(*this, e);
    }
  }

  template <typename Component>
  typename SparseArray<Component>::ReferenceType AddComponent(
      const Entity& to, Component&& component) {
    auto& components = GetComponents<Component>();
    return components.InsertAt(static_cast<std::size_t>(to),
                               std::forward<Component>(component));
  }

  template <typename Component>
  typename SparseArray<Component>::ReferenceType AddComponent(
      const Entity& to, const Component& component) {
    auto& components = GetComponents<Component>();
    return components.InsertAt(static_cast<std::size_t>(to), component);
  }

  template <typename Component, typename... Params>
  typename SparseArray<Component>::ReferenceType EmplaceComponent(
      const Entity& to, Params&&... params) {
    auto& components = GetComponents<Component>();
    return components.EmplaceAt(static_cast<std::size_t>(to),
                                std::forward<Params>(params)...);
  }

  template <typename Component>
  void RemoveComponent(const Entity& from) {
    auto& components = GetComponents<Component>();
    components.Erase(static_cast<std::size_t>(from));
  }

 private:
  std::unordered_map<std::type_index, std::any> components_arrays_;
  std::vector<std::function<void(Registry&, const Entity&)>>
      component_deleters_;
  std::size_t next_entity_id_ = 0;
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_REGISTRY_H_