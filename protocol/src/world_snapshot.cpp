#include "protocol/world_snapshot.h"

#include <limits>

namespace protocol {

namespace {

bool EncodeFullEntityState(const EntityNetState& state,
                           engine::net::PacketBuffer& buffer) {
  buffer.WriteUint16(state.type);
  buffer.WriteInt16(state.x);
  buffer.WriteInt16(state.y);
  buffer.WriteInt16(state.vx);
  buffer.WriteInt16(state.vy);
  buffer.WriteUint8(state.hp);
  buffer.WriteUint8(state.flags);
  return true;
}

bool DecodeFullEntityState(engine::net::PacketBuffer& buffer,
                           EntityNetState& out_state) {
  std::uint16_t type = 0;
  std::int16_t x = 0;
  std::int16_t y = 0;
  std::int16_t vx = 0;
  std::int16_t vy = 0;
  std::uint8_t hp = 0;
  std::uint8_t flags = 0;

  if (!buffer.ReadUint16(type) || !buffer.ReadInt16(x) ||
      !buffer.ReadInt16(y) || !buffer.ReadInt16(vx) || !buffer.ReadInt16(vy) ||
      !buffer.ReadUint8(hp) || !buffer.ReadUint8(flags)) {
    return false;
  }
  out_state.type = type;
  out_state.x = x;
  out_state.y = y;
  out_state.vx = vx;
  out_state.vy = vy;
  out_state.hp = hp;
  out_state.flags = flags;
  return true;
}

}  // namespace

bool EncodeWorldSnapshot(const WorldSnapshotPayload& payload,
                         engine::net::PacketBuffer& buffer) {
  if (payload.deltas.size() >
      static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())) {
    return false;
  }

  buffer.WriteUint32(payload.snapshot_id);
  buffer.WriteUint32(payload.base_snapshot_id);
  buffer.WriteUint32(payload.server_tick);

  const std::uint16_t count = static_cast<std::uint16_t>(payload.deltas.size());
  buffer.WriteUint16(count);

  for (const EntityDelta& delta : payload.deltas) {
    buffer.WriteUint8(static_cast<std::uint8_t>(delta.op));
    buffer.WriteUint32(delta.entity_id);

    switch (delta.op) {
      case EntityDeltaOp::kCreate: {
        EncodeFullEntityState(delta.state, buffer);
        break;
      }

      case EntityDeltaOp::kDelete: {
        break;
      }

      case EntityDeltaOp::kUpdate: {
        const std::uint8_t mask = delta.field_mask;
        buffer.WriteUint8(mask);

        if (mask & kFieldType) {
          buffer.WriteUint16(delta.state.type);
        }
        if (mask & kFieldX) {
          buffer.WriteInt16(delta.state.x);
        }
        if (mask & kFieldY) {
          buffer.WriteInt16(delta.state.y);
        }
        if (mask & kFieldVx) {
          buffer.WriteInt16(delta.state.vx);
        }
        if (mask & kFieldVy) {
          buffer.WriteInt16(delta.state.vy);
        }
        if (mask & kFieldHp) {
          buffer.WriteUint8(delta.state.hp);
        }
        if (mask & kFieldFlags) {
          buffer.WriteUint8(delta.state.flags);
        }
        break;
      }
    }
  }

  return true;
}

bool DecodeWorldSnapshot(engine::net::PacketBuffer& buffer,
                         WorldSnapshotPayload& out_payload) {
  WorldSnapshotPayload result;

  std::uint32_t snapshot_id = 0;
  std::uint32_t base_snapshot_id = 0;
  std::uint32_t server_tick = 0;
  std::uint16_t count = 0;

  if (!buffer.ReadUint32(snapshot_id) || !buffer.ReadUint32(base_snapshot_id) ||
      !buffer.ReadUint32(server_tick) || !buffer.ReadUint16(count)) {
    return false;
  }

  result.snapshot_id = snapshot_id;
  result.base_snapshot_id = base_snapshot_id;
  result.server_tick = server_tick;
  result.deltas.clear();
  result.deltas.reserve(count);

  for (std::uint16_t i = 0; i < count; ++i) {
    std::uint8_t op_code = 0;
    std::uint32_t entity_id = 0;

    if (!buffer.ReadUint8(op_code) || !buffer.ReadUint32(entity_id)) {
      return false;
    }

    EntityDelta delta;
    delta.op = static_cast<EntityDeltaOp>(op_code);
    delta.entity_id = entity_id;
    delta.field_mask = 0;

    EntityNetState state{};
    state.entity_id = entity_id;

    switch (delta.op) {
      case EntityDeltaOp::kCreate: {
        if (!DecodeFullEntityState(buffer, state)) {
          return false;
        }
        break;
      }

      case EntityDeltaOp::kDelete: {
        break;
      }

      case EntityDeltaOp::kUpdate: {
        std::uint8_t mask = 0;
        if (!buffer.ReadUint8(mask)) {
          return false;
        }
        delta.field_mask = mask;

        if (mask & kFieldType) {
          std::uint16_t type = 0;
          if (!buffer.ReadUint16(type)) {
            return false;
          }
          state.type = type;
        }
        if (mask & kFieldX) {
          std::int16_t x = 0;
          if (!buffer.ReadInt16(x)) {
            return false;
          }
          state.x = x;
        }
        if (mask & kFieldY) {
          std::int16_t y = 0;
          if (!buffer.ReadInt16(y)) {
            return false;
          }
          state.y = y;
        }
        if (mask & kFieldVx) {
          std::int16_t vx = 0;
          if (!buffer.ReadInt16(vx)) {
            return false;
          }
          state.vx = vx;
        }
        if (mask & kFieldVy) {
          std::int16_t vy = 0;
          if (!buffer.ReadInt16(vy)) {
            return false;
          }
          state.vy = vy;
        }
        if (mask & kFieldHp) {
          std::uint8_t hp = 0;
          if (!buffer.ReadUint8(hp)) {
            return false;
          }
          state.hp = hp;
        }
        if (mask & kFieldFlags) {
          std::uint8_t flags = 0;
          if (!buffer.ReadUint8(flags)) {
            return false;
          }
          state.flags = flags;
        }
        break;
      }
    }
    delta.state = state;
    result.deltas.push_back(std::move(delta));
  }
  out_payload = std::move(result);
  return true;
}

}  // namespace protocol
