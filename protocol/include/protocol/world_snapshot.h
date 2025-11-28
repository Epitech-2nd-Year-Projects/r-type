#ifndef PROTOCOL_WORLD_SNAPSHOT_H_
#define PROTOCOL_WORLD_SNAPSHOT_H_

#include <cstdint>
#include <vector>

#include "engine/net/packet_buffer.h"

namespace protocol {

/// @brief Special value meaning "no base snapshot" (full snapshot).
inline constexpr std::uint32_t kNoBaseSnapshotId = 0xFFFFFFFFu;

/**
 * @brief Compact representation of an entity's network-relevant state.
 * 
 * Quantization is handled by game/engine code before filling this struct.
 */
struct EntityNetState {
  std::uint32_t entity_id = 0;  ///< Stable ID for the entity.
  std::uint16_t type = 0;       ///< Archetype code (player, enemy, missile, ...).
  std::int16_t x = 0;           ///< Quantized position X.
  std::int16_t y = 0;           ///< Quantized position Y.
  std::int16_t vx = 0;          ///< Quantized velocity X.
  std::int16_t vy = 0;          ///< Quantized velocity Y.
  std::uint8_t hp = 0;          ///< Hit points (0 = dead).
  std::uint8_t flags = 0;       ///< Status flags (alive, invincible, shield, etc.).
};

/**
 * @brief Type of per-entity operation in a delta snapshot.
 */
enum class EntityDeltaOp : std::uint8_t {
  kCreate = 0,  ///< Create a new entity.
  kUpdate = 1,  ///< Update existing entity.
  kDelete = 2,  ///< Delete an entity.
};

/**
 * @brief Bitmask for fields that may be present in an update delta.
 * 
 * The mask applies to EntityNetState fields for the entity.
 */
enum EntityFieldMask : std::uint8_t {
  kFieldType = 1u << 0,   ///< Entity type field mask.
  kFieldX = 1u << 1,      ///< Position X field mask.
  kFieldY = 1u << 2,      ///< Position Y field mask.
  kFieldVx = 1u << 3,     ///< Velocity X field mask.
  kFieldVy = 1u << 4,     ///< Velocity Y field mask.
  kFieldHp = 1u << 5,     ///< Hit points field mask.
  kFieldFlags = 1u << 6,  ///< Flags field mask.
};

/**
 * @brief Representation of a single entity delta in a snapshot payload.
 * 
 * Semantics:
 *   - For kCreate:
 *       - entity_id and all fields in state are meaningful.
 *       - field_mask is typically 0 (ignored).
 *   - For kDelete:
 *       - Only entity_id and op are meaningful.
 *       - field_mask and state are ignored by higher layers.
 *   - For kUpdate:
 *       - entity_id and field_mask are meaningful.
 *       - In state, only fields whose bit is set in field_mask are meaningful.
 *         Other fields should be treated as "unset".
 */
struct EntityDelta {
  EntityDeltaOp op = EntityDeltaOp::kCreate;  ///< Operation type (create/update/delete).
  std::uint32_t entity_id = 0;                ///< Entity identifier.
  std::uint8_t field_mask = 0;                ///< Bitmask of fields present in update.
  EntityNetState state{};                     ///< Entity state data.
};

/**
 * @brief Payload for MessageType::kWorldSnapshot.
 * 
 * Wire format:
 *   - uint32  snapshot_id
 *   - uint32  base_snapshot_id  (kNoBaseSnapshotId for full snapshot)
 *   - uint32  server_tick
 *   - uint16  entity_delta_count
 *   - [entity_delta_count x EntityDelta encoded as below]
 * 
 * EntityDelta encoding:
 * 
 *   - uint8  op
 *   - uint32 entity_id
 * 
 *   if op == kCreate:
 *     - uint16 type
 *     - int16  x
 *     - int16  y
 *     - int16  vx
 *     - int16  vy
 *     - uint8  hp
 *     - uint8  flags
 * 
 *   if op == kDelete:
 *     - (no additional data)
 * 
 *   if op == kUpdate:
 *     - uint8  field_mask
 *     - [if field_mask & kFieldType]  uint16 type
 *     - [if field_mask & kFieldX]     int16  x
 *     - [if field_mask & kFieldY]     int16  y
 *     - [if field_mask & kFieldVx]    int16  vx
 *     - [if field_mask & kFieldVy]    int16  vy
 *     - [if field_mask & kFieldHp]    uint8  hp
 *     - [if field_mask & kFieldFlags] uint8  flags
 */
struct WorldSnapshotPayload {
  std::uint32_t snapshot_id = 0;                         ///< Current snapshot identifier.
  std::uint32_t base_snapshot_id = kNoBaseSnapshotId;   ///< Base snapshot for delta (or kNoBaseSnapshotId).
  std::uint32_t server_tick = 0;                         ///< Server simulation tick.
  std::vector<EntityDelta> deltas;                       ///< Entity deltas in this snapshot.
};

/**
 * @brief Encodes a WorldSnapshotPayload into the buffer.
 * @param payload The world snapshot to serialize.
 * @param buffer The packet buffer to write to.
 * @return false if the number of deltas exceeds what fits in uint16, true otherwise.
 */
bool EncodeWorldSnapshot(const WorldSnapshotPayload& payload,
                         engine::net::PacketBuffer& buffer);

/**
 * @brief Decodes a WorldSnapshotPayload from the buffer.
 * @param buffer The packet buffer to read from.
 * @param out_payload Output parameter for the deserialized world snapshot.
 * @return false if the buffer does not contain a valid snapshot, true otherwise.
 */
bool DecodeWorldSnapshot(engine::net::PacketBuffer& buffer,
                         WorldSnapshotPayload& out_payload);

}  // namespace protocol

#endif  // PROTOCOL_WORLD_SNAPSHOT_H_
