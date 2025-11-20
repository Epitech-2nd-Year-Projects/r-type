#ifndef ENGINE_ECS_ENTITY_ID_H_
#define ENGINE_ECS_ENTITY_ID_H_

namespace engine::ecs {

class EntityId {
 private:
  std::size_t id_;
  friend class Registry;

  explicit EntityId(std::size_t id) noexcept : id_(id) {}

 public:
  operator std::size_t() const noexcept { return id_; }

  bool operator==(const EntityId &other) const noexcept {
    return id_ == other.id_;
  }
  bool operator!=(const EntityId &other) const noexcept {
    return id_ != other.id_;
  }
  bool operator<(const EntityId &other) const noexcept {
    return id_ < other.id_;
  }
};
}  // namespace engine::ecs

#endif /* !ENGINE_ECS_ENTITY_ID_H_ */