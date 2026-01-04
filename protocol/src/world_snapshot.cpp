#include "protocol/world_snapshot.h"

#include <limits>
#include <unordered_map>
#include <unordered_set>

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
  buffer.WriteUint32(state.score);
  buffer.WriteUint8(state.lives);
  buffer.WriteUint32(state.player_id);
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
  std::uint32_t score = 0;
  std::uint8_t lives = 0;
  std::uint32_t player_id = 0;

  if (!buffer.ReadUint16(type) || !buffer.ReadInt16(x) ||
      !buffer.ReadInt16(y) || !buffer.ReadInt16(vx) || !buffer.ReadInt16(vy) ||
      !buffer.ReadUint8(hp) || !buffer.ReadUint8(flags) ||
      !buffer.ReadUint32(score) || !buffer.ReadUint8(lives) ||
      !buffer.ReadUint32(player_id)) {
    return false;
  }
  out_state.type = type;
  out_state.x = x;
  out_state.y = y;
  out_state.vx = vx;
  out_state.vy = vy;
  out_state.hp = hp;
  out_state.flags = flags;
  out_state.score = score;
  out_state.lives = lives;
  out_state.player_id = player_id;
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
  buffer.WriteUint32(payload.current_wave);

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
        const std::uint16_t mask = delta.field_mask;
        buffer.WriteUint16(mask);

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
        if (mask & kFieldScore) {
          buffer.WriteUint32(delta.state.score);
        }
        if (mask & kFieldLives) {
          buffer.WriteUint8(delta.state.lives);
        }
        if (mask & kFieldPlayerId) {
          buffer.WriteUint32(delta.state.player_id);
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
  std::uint32_t current_wave = 0;
  std::uint16_t count = 0;

  if (!buffer.ReadUint32(snapshot_id) || !buffer.ReadUint32(base_snapshot_id) ||
      !buffer.ReadUint32(server_tick) || !buffer.ReadUint32(current_wave) ||
      !buffer.ReadUint16(count)) {
    return false;
  }

  result.snapshot_id = snapshot_id;
  result.base_snapshot_id = base_snapshot_id;
  result.server_tick = server_tick;
  result.current_wave = current_wave;
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
        std::uint16_t mask = 0;
        if (!buffer.ReadUint16(mask)) {
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
        if (mask & kFieldScore) {
          std::uint32_t score = 0;
          if (!buffer.ReadUint32(score)) {
            return false;
          }
          state.score = score;
        }
        if (mask & kFieldLives) {
          std::uint8_t lives = 0;
          if (!buffer.ReadUint8(lives)) {
            return false;
          }
          state.lives = lives;
        }
        if (mask & kFieldPlayerId) {
          std::uint32_t player_id = 0;
          if (!buffer.ReadUint32(player_id)) {
            return false;
          }
          state.player_id = player_id;
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

WorldSnapshotPayload ComputeDelta(const WorldSnapshotPayload& current,
                                  const WorldSnapshotPayload& base) {
  WorldSnapshotPayload result;
  result.snapshot_id = current.snapshot_id;
  result.base_snapshot_id = base.snapshot_id;
  result.server_tick = current.server_tick;
  result.current_wave = current.current_wave;

  std::unordered_map<std::uint32_t, const EntityNetState*> base_entities;
  for (const auto& delta : base.deltas) {
    if (delta.op == EntityDeltaOp::kCreate ||
        delta.op == EntityDeltaOp::kUpdate) {
      base_entities[delta.entity_id] = &delta.state;
    }
  }

  for (const auto& curr_delta : current.deltas) {
    if (curr_delta.op != EntityDeltaOp::kCreate) {
      continue;
    }
    const auto& curr_state = curr_delta.state;
    auto it = base_entities.find(curr_state.entity_id);

    if (it == base_entities.end()) {
      result.deltas.push_back(curr_delta);
    } else {
      const auto& base_state = *it->second;
      std::uint16_t mask = 0;

      if (curr_state.type != base_state.type) mask |= kFieldType;
      if (curr_state.x != base_state.x) mask |= kFieldX;
      if (curr_state.y != base_state.y) mask |= kFieldY;
      if (curr_state.vx != base_state.vx) mask |= kFieldVx;
      if (curr_state.vy != base_state.vy) mask |= kFieldVy;
      if (curr_state.hp != base_state.hp) mask |= kFieldHp;
      if (curr_state.flags != base_state.flags) mask |= kFieldFlags;
      if (curr_state.score != base_state.score) mask |= kFieldScore;
      if (curr_state.lives != base_state.lives) mask |= kFieldLives;
      if (curr_state.player_id != base_state.player_id) mask |= kFieldPlayerId;

      if (mask != 0) {
        EntityDelta update_delta;
        update_delta.op = EntityDeltaOp::kUpdate;
        update_delta.entity_id = curr_state.entity_id;
        update_delta.field_mask = mask;
        update_delta.state = curr_state;
        result.deltas.push_back(std::move(update_delta));
      }
      base_entities.erase(it);
    }
  }

  for (const auto& [id, _] : base_entities) {
    EntityDelta delete_delta;
    delete_delta.op = EntityDeltaOp::kDelete;
    delete_delta.entity_id = id;
    result.deltas.push_back(std::move(delete_delta));
  }

  return result;
}

WorldSnapshotPayload ApplyDelta(const WorldSnapshotPayload& base,
                                const WorldSnapshotPayload& delta) {
  WorldSnapshotPayload result;
  result.snapshot_id = delta.snapshot_id;
  result.base_snapshot_id = kNoBaseSnapshotId;
  result.server_tick = delta.server_tick;
  result.current_wave = delta.current_wave;

  // Map base entities
  std::unordered_map<std::uint32_t, const EntityNetState*> base_entities;
  for (const auto& d : base.deltas) {
    if (d.op == EntityDeltaOp::kCreate) {
      base_entities[d.entity_id] = &d.state;
    }
  }

  std::unordered_set<std::uint32_t> processed_ids;

  for (const auto& d : delta.deltas) {
    EntityDelta new_delta;
    new_delta.op = EntityDeltaOp::kCreate;
    new_delta.entity_id = d.entity_id;

    if (d.op == EntityDeltaOp::kCreate) {
      new_delta.state = d.state;
      result.deltas.push_back(new_delta);
      processed_ids.insert(d.entity_id);
    } else if (d.op == EntityDeltaOp::kUpdate) {
      auto it = base_entities.find(d.entity_id);
      if (it != base_entities.end()) {
        EntityNetState merged = *it->second;
        if (d.field_mask & kFieldType) merged.type = d.state.type;
        if (d.field_mask & kFieldX) merged.x = d.state.x;
        if (d.field_mask & kFieldY) merged.y = d.state.y;
        if (d.field_mask & kFieldVx) merged.vx = d.state.vx;
        if (d.field_mask & kFieldVy) merged.vy = d.state.vy;
        if (d.field_mask & kFieldHp) merged.hp = d.state.hp;
        if (d.field_mask & kFieldFlags) merged.flags = d.state.flags;
        if (d.field_mask & kFieldScore) merged.score = d.state.score;
        if (d.field_mask & kFieldLives) merged.lives = d.state.lives;
        if (d.field_mask & kFieldPlayerId) merged.player_id = d.state.player_id;

        new_delta.state = merged;
        result.deltas.push_back(new_delta);
        processed_ids.insert(d.entity_id);
      }
    } else if (d.op == EntityDeltaOp::kDelete) {
      processed_ids.insert(d.entity_id);
    }
  }

  for (const auto& [id, state_ptr] : base_entities) {
    if (processed_ids.find(id) == processed_ids.end()) {
      EntityDelta unchanged;
      unchanged.op = EntityDeltaOp::kCreate;
      unchanged.entity_id = id;
      unchanged.state = *state_ptr;
      result.deltas.push_back(unchanged);
    }
  }

  return result;
}

}  // namespace protocol
