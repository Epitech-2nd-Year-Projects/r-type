#ifndef ENGINE_ECS_ENTITY_ID_H_
#define ENGINE_ECS_ENTITY_ID_H_

namespace engine::ecs {

class EntityId {
 private:
  std::size_t id_;
  friend class registry;

  explicit EntityId(std::size_t id) noexcept : id(id) {}

 public:
  operator std::size_t() const noexcept { return id; }

  bool operator==(const Entity &other) const noexcept { return id == other.id; }
  bool operator!=(const Entity &other) const noexcept { return id != other.id; }
  bool operator<(const Entity &other) const noexcept { return id < other.id; }
};
}  // namespace engine::ecs

#endif /* !ENGINE_ECS_ENTITY_ID_H_ */