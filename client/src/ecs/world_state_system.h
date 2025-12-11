/**
 * @file world_state_system.h
 * @brief Applies network world snapshots to the client ECS
 *
 * @details
 * Consumes protocol world snapshots and mutates the client registry by
 * creating, updating, and destroying entities. Maintains a mapping between
 * network entity identifiers and local EntityId instances to support delta
 * processing and resynchronization on full snapshots.
 */

#ifndef CLIENT_ECS_WORLD_STATE_SYSTEM_H_
#define CLIENT_ECS_WORLD_STATE_SYSTEM_H_

#include <cstdint>
#include <unordered_map>
#include "ecs/components.h"
#include "engine/ecs/registry.h"
#include "protocol/world_snapshot.h"

namespace client::ecs {

/**
 * @class WorldStateSystem
 * @brief Applies server snapshots to the local ECS registry
 *
 * @details
 * Maintains entity mappings and applies create/update/delete operations from
 * WorldSnapshotPayload instances. Full snapshots (base_snapshot_id ==
 * kNoBaseSnapshotId) will prune entities not present in the incoming payload.
 * Snapshot ordering is enforced via snapshot_id to avoid stale rewrites.
 */
class WorldStateSystem {
 public:
  explicit WorldStateSystem(engine::ecs::Registry& registry);

  /**
   * @brief Clear all tracked network entities and component data
   */
  void Reset();

  /**
   * @brief Apply a world snapshot to the registry
   * @param snapshot Decoded snapshot payload from the server
   */
  void ApplySnapshot(const protocol::WorldSnapshotPayload& snapshot);

  /**
   * @brief Last applied snapshot identifier
   */
  std::uint32_t last_snapshot_id() const { return last_snapshot_id_; }

 private:
  void RegisterComponents();
  void ApplyCreate(const protocol::EntityDelta& delta,
                   std::uint32_t snapshot_id);
  void ApplyUpdate(const protocol::EntityDelta& delta,
                   std::uint32_t snapshot_id);
  void ApplyDelete(const protocol::EntityDelta& delta);
  engine::ecs::EntityId ResolveOrCreateEntity(std::uint32_t network_id,
                                              std::uint32_t snapshot_id,
                                              std::uint16_t type_code);
  static engine::math::Vector2f ToVector(std::int16_t x, std::int16_t y);

  engine::ecs::Registry& registry_;
  std::unordered_map<std::uint32_t, engine::ecs::EntityId> network_to_entity_;
  std::uint32_t last_snapshot_id_{0};
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_WORLD_STATE_SYSTEM_H_
