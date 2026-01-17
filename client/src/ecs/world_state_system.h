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
#include <optional>
#include <unordered_map>

#include "ecs/animation_factory.h"
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
   * @param receipt_timestamp_ms Monotonic timestamp when the snapshot was
   *        received locally
   * @param local_player_id Optional local player id to tag local entity
   * @param server_tick_rate_hz Optional server tick rate for interpolation
   * @param snapshot_time_ms Optional local-time estimate of the snapshot
   *        (server timestamp converted into client time)
   */
  void ApplySnapshot(
      const protocol::WorldSnapshotPayload& snapshot,
      std::uint64_t receipt_timestamp_ms,
      std::optional<std::uint32_t> local_player_id = std::nullopt,
      std::optional<std::uint32_t> server_tick_rate_hz = std::nullopt,
      std::optional<std::uint64_t> snapshot_time_ms = std::nullopt);

  /**
   * @brief Last applied snapshot identifier
   */
  std::uint32_t last_snapshot_id() const { return last_snapshot_id_; }

 private:
  void RegisterComponents();
  void ApplyCreate(const protocol::EntityDelta& delta,
                   std::uint32_t snapshot_id,
                   std::uint64_t receipt_timestamp_ms,
                   std::optional<std::uint64_t> snapshot_time_ms);
  void ApplyUpdate(const protocol::EntityDelta& delta,
                   std::uint32_t snapshot_id,
                   std::uint64_t receipt_timestamp_ms,
                   std::optional<std::uint64_t> snapshot_time_ms);

  void ApplyDelete(const protocol::EntityDelta& delta);
  engine::ecs::EntityId ResolveOrCreateEntity(std::uint32_t network_id,
                                              std::uint32_t snapshot_id,
                                              std::uint16_t type_code);
  void UpdateArchetypeTags(engine::ecs::EntityId entity,
                           std::uint16_t type_code);
  static engine::math::Vector2f ToVector(std::int16_t x, std::int16_t y);
  void UpdateLocalPlayerTag(engine::ecs::EntityId entity,
                            std::optional<std::uint32_t> local_player_id);
  std::optional<std::uint64_t> ResolveSnapshotTimeMs(
      const protocol::WorldSnapshotPayload& snapshot,
      std::optional<std::uint32_t> server_tick_rate_hz,
      std::optional<std::uint64_t> snapshot_time_ms) const;

  engine::ecs::Registry& registry_;
  const ArchetypeRegistry& archetypes_;
  AnimationFactory animation_factory_;
  std::unordered_map<std::uint32_t, engine::ecs::EntityId> network_to_entity_;
  std::uint32_t last_snapshot_id_{0};
  std::optional<engine::ecs::EntityId> local_player_entity_;
  std::optional<std::uint32_t> server_tick_rate_hz_;
  std::optional<std::uint64_t> server_time_anchor_ms_;
  std::optional<std::uint64_t> server_tick_anchor_;
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_WORLD_STATE_SYSTEM_H_
