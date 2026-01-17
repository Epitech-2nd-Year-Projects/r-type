#ifndef RIFT_CLIENT_ECS_WORLD_STATE_SYSTEM_H_
#define RIFT_CLIENT_ECS_WORLD_STATE_SYSTEM_H_

#include <cstdint>
#include <optional>
#include <unordered_map>

#include "engine/ecs/registry.h"
#include "protocol/world_snapshot.h"

namespace rift::client::ecs {

/// @brief Entity type codes sent by the server.
enum class EntityType : std::uint16_t {
  kUnknown = 0,
  kFighter = 1,
  kHitbox = 2,
  kEffect = 3,
};

/// @brief Processes world snapshots and synchronizes ECS entities.
///
/// This system receives WorldSnapshotPayload from the server and creates,
/// updates, or deletes local ECS entities accordingly. It maintains a mapping
/// from network entity IDs to local ECS entity IDs.
class WorldStateSystem {
 public:
  /// @brief Constructs the system with a reference to the ECS registry.
  /// @param registry The ECS registry to manage entities in.
  explicit WorldStateSystem(engine::ecs::Registry& registry);

  /// @brief Resets the system, removing all tracked entities.
  void Reset();

  /// @brief Applies a world snapshot, creating/updating/deleting entities.
  /// @param snapshot The snapshot payload from the server.
  /// @param local_player_id The local player's ID (to tag local fighter).
  /// @param receipt_timestamp_ms Timestamp when snapshot was received.
  void ApplySnapshot(const protocol::WorldSnapshotPayload& snapshot,
                     std::optional<std::uint32_t> local_player_id,
                     std::uint64_t receipt_timestamp_ms);

  /// @brief Returns the last processed snapshot ID.
  std::uint32_t last_snapshot_id() const { return last_snapshot_id_; }

 private:
  void ApplyCreate(const protocol::EntityDelta& delta,
                   std::optional<std::uint32_t> local_player_id,
                   std::uint32_t snapshot_id,
                   std::uint64_t receipt_timestamp_ms);

  void ApplyUpdate(const protocol::EntityDelta& delta,
                   std::optional<std::uint32_t> local_player_id,
                   std::uint32_t snapshot_id,
                   std::uint64_t receipt_timestamp_ms);

  void ApplyDelete(const protocol::EntityDelta& delta);

  engine::ecs::EntityId ResolveOrCreateEntity(std::uint32_t network_id,
                                               std::uint32_t snapshot_id,
                                               std::uint16_t type_code);

  void SetupFighterComponents(engine::ecs::EntityId entity,
                              const protocol::EntityNetState& state,
                              std::optional<std::uint32_t> local_player_id);

  engine::ecs::Registry& registry_;
  std::unordered_map<std::uint32_t, engine::ecs::EntityId> network_to_entity_;
  std::uint32_t last_snapshot_id_{0};
};

}  // namespace rift::client::ecs

#endif  // RIFT_CLIENT_ECS_WORLD_STATE_SYSTEM_H_
