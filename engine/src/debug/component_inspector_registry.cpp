#include "engine/debug/component_inspector_registry.h"

namespace engine::debug {

std::optional<std::reference_wrapper<const ComponentMeta>>
ComponentInspectorRegistry::Get(std::type_index type) const {
  auto it = inspectors_.find(type);
  if (it == inspectors_.end()) {
    return std::nullopt;
  }
  return std::cref(it->second);
}

bool ComponentInspectorRegistry::HasInspector(std::type_index type) const {
  return inspectors_.find(type) != inspectors_.end();
}

const std::unordered_map<std::type_index, ComponentMeta>&
ComponentInspectorRegistry::All() const {
  return inspectors_;
}

std::size_t ComponentInspectorRegistry::Size() const noexcept {
  return inspectors_.size();
}

}  // namespace engine::debug
