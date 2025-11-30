#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "engine/net/packet_buffer.h"
#include "protocol/command.h"
#include "protocol/header.h"
#include "protocol/input_state.h"
#include "protocol/join.h"
#include "protocol/packet.h"
#include "protocol/ping.h"
#include "protocol/player_died.h"
#include "protocol/world_snapshot.h"

namespace {

template <typename F>
bool RunTest(const char* name, F&& fn) {
  const bool ok = fn();
  std::cout << (ok ? "[ OK ] " : "[FAIL] ") << name << "\n";
  return ok;
}

bool TestInputStateRoundTrip() {
  engine::net::PacketBuffer buffer;

  protocol::InputStatePayload original{};
  original.command_count = 1;
  original.commands[0].input_sequence = 42u;
  original.commands[0].buttons = protocol::kInputUp | protocol::kInputFire;
  original.commands[0].analog_x = 123;
  original.commands[0].analog_y = -45;
  original.commands[0].client_time_ms = 1337u;

  if (!protocol::EncodeInputState(original, buffer)) {
    return false;
  }

  engine::net::PacketBuffer read_buffer(buffer.storage());
  protocol::InputStatePayload decoded{};

  if (!protocol::DecodeInputState(read_buffer, decoded)) {
    return false;
  }
  if (decoded.command_count != original.command_count) {
    return false;
  }

  return decoded.commands[0].input_sequence ==
             original.commands[0].input_sequence &&
         decoded.commands[0].buttons == original.commands[0].buttons &&
         decoded.commands[0].analog_x == original.commands[0].analog_x &&
         decoded.commands[0].analog_y == original.commands[0].analog_y &&
         decoded.commands[0].client_time_ms ==
             original.commands[0].client_time_ms;
}

bool TestPingPongRoundTrip() {
  engine::net::PacketBuffer buffer;

  protocol::PingPayload ping{};
  ping.client_time_ms = 555u;

  if (!protocol::EncodePing(ping, buffer)) {
    return false;
  }

  engine::net::PacketBuffer read_ping(buffer.storage());
  protocol::PingPayload decoded_ping{};
  if (!protocol::DecodePing(read_ping, decoded_ping)) {
    return false;
  }
  if (decoded_ping.client_time_ms != ping.client_time_ms) {
    return false;
  }

  engine::net::PacketBuffer buffer2;
  protocol::PongPayload pong{};
  pong.client_time_ms = 555u;
  pong.server_time_ms = 999u;

  if (!protocol::EncodePong(pong, buffer2)) {
    return false;
  }

  engine::net::PacketBuffer read_pong(buffer2.storage());
  protocol::PongPayload decoded_pong{};
  if (!protocol::DecodePong(read_pong, decoded_pong)) {
    return false;
  }

  return decoded_pong.client_time_ms == pong.client_time_ms &&
         decoded_pong.server_time_ms == pong.server_time_ms;
}

bool TestJoinRoundTrip() {
  {
    engine::net::PacketBuffer buffer;

    protocol::JoinRequestPayload original{};
    original.client_version = 1;
    original.player_name = "Lindon";
    original.room_code = "ROOM42";

    if (!protocol::EncodeJoinRequest(original, buffer)) {
      return false;
    }

    engine::net::PacketBuffer read_buffer(buffer.storage());
    protocol::JoinRequestPayload decoded{};
    if (!protocol::DecodeJoinRequest(read_buffer, decoded)) {
      return false;
    }

    if (decoded.client_version != original.client_version ||
        decoded.player_name != original.player_name ||
        decoded.room_code != original.room_code) {
      return false;
    }
  }

  {
    engine::net::PacketBuffer buffer;

    protocol::JoinAcceptPayload original{};
    original.server_version = 1;
    original.player_id = 123u;
    original.max_players = 4u;
    original.tick_rate = 60u;
    original.seed = 0xDEADBEEFu;

    if (!protocol::EncodeJoinAccept(original, buffer)) {
      return false;
    }

    engine::net::PacketBuffer read_buffer(buffer.storage());
    protocol::JoinAcceptPayload decoded{};
    if (!protocol::DecodeJoinAccept(read_buffer, decoded)) {
      return false;
    }

    if (decoded.server_version != original.server_version ||
        decoded.player_id != original.player_id ||
        decoded.max_players != original.max_players ||
        decoded.tick_rate != original.tick_rate ||
        decoded.seed != original.seed) {
      return false;
    }
  }

  {
    engine::net::PacketBuffer buffer;

    protocol::JoinRejectPayload original{};
    original.server_version = 1;
    original.reason = protocol::JoinRejectReason::kServerFull;
    original.message = "Server is full";

    if (!protocol::EncodeJoinReject(original, buffer)) {
      return false;
    }

    engine::net::PacketBuffer read_buffer(buffer.storage());
    protocol::JoinRejectPayload decoded{};
    if (!protocol::DecodeJoinReject(read_buffer, decoded)) {
      return false;
    }

    if (decoded.server_version != original.server_version ||
        decoded.reason != original.reason ||
        decoded.message != original.message) {
      return false;
    }
  }

  return true;
}

bool TestPlayerDiedRoundTrip() {
  engine::net::PacketBuffer buffer;

  protocol::PlayerDiedPayload original{};
  original.player_id = 7u;
  original.killer_entity_id = 999u;
  original.cause = protocol::DeathCause::kProjectile;
  original.remaining_lives = 2u;

  if (!protocol::EncodePlayerDied(original, buffer)) {
    return false;
  }

  engine::net::PacketBuffer read_buffer(buffer.storage());
  protocol::PlayerDiedPayload decoded{};
  if (!protocol::DecodePlayerDied(read_buffer, decoded)) {
    return false;
  }

  return decoded.player_id == original.player_id &&
         decoded.killer_entity_id == original.killer_entity_id &&
         decoded.cause == original.cause &&
         decoded.remaining_lives == original.remaining_lives;
}

bool TestCommandRoundTrip() {
  engine::net::PacketBuffer buffer;

  protocol::CommandPayload original{};
  original.command_id =
      static_cast<std::uint16_t>(protocol::CommandType::kChatMessage);
  original.payload = R"({"text":"hello world"})";

  if (!protocol::EncodeCommand(original, buffer)) {
    return false;
  }

  engine::net::PacketBuffer read_buffer(buffer.storage());
  protocol::CommandPayload decoded{};
  if (!protocol::DecodeCommand(read_buffer, decoded)) {
    return false;
  }

  return decoded.command_id == original.command_id &&
         decoded.payload == original.payload;
}

bool TestWorldSnapshotRoundTrip() {
  engine::net::PacketBuffer buffer;

  protocol::WorldSnapshotPayload original{};
  original.snapshot_id = 10u;
  original.base_snapshot_id = protocol::kNoBaseSnapshotId;
  original.server_tick = 1234u;

  {
    protocol::EntityDelta d{};
    d.op = protocol::EntityDeltaOp::kCreate;
    d.entity_id = 1u;
    d.field_mask = 0;
    d.state.entity_id = 1u;
    d.state.type = 2u;
    d.state.x = 100;
    d.state.y = 200;
    d.state.vx = 5;
    d.state.vy = -3;
    d.state.hp = 10;
    d.state.flags = 0;
    original.deltas.push_back(d);
  }

  {
    protocol::EntityDelta d{};
    d.op = protocol::EntityDeltaOp::kUpdate;
    d.entity_id = 1u;
    d.field_mask = protocol::kFieldX | protocol::kFieldHp;
    d.state.entity_id = 1u;
    d.state.x = 150;
    d.state.hp = 8;
    original.deltas.push_back(d);
  }

  {
    protocol::EntityDelta d{};
    d.op = protocol::EntityDeltaOp::kDelete;
    d.entity_id = 2u;
    original.deltas.push_back(d);
  }

  if (!protocol::EncodeWorldSnapshot(original, buffer)) {
    return false;
  }

  engine::net::PacketBuffer read_buffer(buffer.storage());
  protocol::WorldSnapshotPayload decoded{};
  if (!protocol::DecodeWorldSnapshot(read_buffer, decoded)) {
    return false;
  }

  if (decoded.snapshot_id != original.snapshot_id ||
      decoded.base_snapshot_id != original.base_snapshot_id ||
      decoded.server_tick != original.server_tick ||
      decoded.deltas.size() != original.deltas.size()) {
    return false;
  }

  if (decoded.deltas.size() != 3u) {
    return false;
  }

  {
    const protocol::EntityDelta& src = original.deltas[0];
    const protocol::EntityDelta& dst = decoded.deltas[0];

    if (dst.op != protocol::EntityDeltaOp::kCreate) {
      return false;
    }
    if (dst.entity_id != src.entity_id) {
      return false;
    }

    if (dst.state.entity_id != src.state.entity_id ||
        dst.state.type != src.state.type || dst.state.x != src.state.x ||
        dst.state.y != src.state.y || dst.state.vx != src.state.vx ||
        dst.state.vy != src.state.vy || dst.state.hp != src.state.hp ||
        dst.state.flags != src.state.flags) {
      return false;
    }

    if (dst.field_mask != 0u) {
      return false;
    }
  }

  {
    const protocol::EntityDelta& src = original.deltas[1];
    const protocol::EntityDelta& dst = decoded.deltas[1];

    if (dst.op != protocol::EntityDeltaOp::kUpdate) {
      return false;
    }
    if (dst.entity_id != src.entity_id) {
      return false;
    }

    if (dst.field_mask != src.field_mask) {
      return false;
    }

    if (!(dst.field_mask & protocol::kFieldX)) {
      return false;
    }
    if (dst.state.x != src.state.x) {
      return false;
    }

    if (!(dst.field_mask & protocol::kFieldHp)) {
      return false;
    }
    if (dst.state.hp != src.state.hp) {
      return false;
    }

    if (dst.state.type != 0 || dst.state.y != 0 || dst.state.vx != 0 ||
        dst.state.vy != 0 || dst.state.flags != 0) {
      return false;
    }
  }

  {
    const protocol::EntityDelta& src = original.deltas[2];
    const protocol::EntityDelta& dst = decoded.deltas[2];

    if (dst.op != protocol::EntityDeltaOp::kDelete) {
      return false;
    }
    if (dst.entity_id != src.entity_id) {
      return false;
    }

    if (dst.field_mask != 0u) {
      return false;
    }
    if (dst.state.entity_id != src.entity_id) {
      return false;
    }

    if (dst.state.type != 0 || dst.state.x != 0 || dst.state.y != 0 ||
        dst.state.vx != 0 || dst.state.vy != 0 || dst.state.hp != 0 ||
        dst.state.flags != 0) {
      return false;
    }
  }

  return true;
}

bool TestPacketRoundTripInputState() {
  protocol::InputStatePayload input{};
  input.command_count = 1;
  input.commands[0].input_sequence = 42u;
  input.commands[0].buttons = protocol::kInputRight | protocol::kInputFire;
  input.commands[0].analog_x = 123;
  input.commands[0].analog_y = -456;
  input.commands[0].client_time_ms = 1337u;

  protocol::Packet original{};
  original.header.version = protocol::kProtocolVersion;
  original.header.message_type = static_cast<std::uint8_t>(
      protocol::message_type::MessageType::kInputState);
  original.header.flags = 0;
  original.header.sequence = 10u;
  original.header.ack = 8u;
  original.header.ack_bits = 0x00000003u;
  original.header.timestamp_ms = 5000u;
  original.payload = input;

  engine::net::PacketBuffer buffer;
  if (!protocol::EncodePacket(original, buffer)) {
    std::cout << "EncodePacket(InputState) returned false\n";
    return false;
  }

  engine::net::PacketBuffer read_buffer(buffer.storage());
  protocol::Packet decoded{};
  if (!protocol::DecodePacket(read_buffer, decoded)) {
    std::cout << "DecodePacket(InputState) returned false\n";
    return false;
  }

  if (decoded.header.version != original.header.version ||
      decoded.header.message_type != original.header.message_type ||
      decoded.header.flags != original.header.flags ||
      decoded.header.sequence != original.header.sequence ||
      decoded.header.ack != original.header.ack ||
      decoded.header.ack_bits != original.header.ack_bits ||
      decoded.header.timestamp_ms != original.header.timestamp_ms) {
    std::cout << "Header mismatch in Packet InputState round-trip\n";
    return false;
  }

  if (!std::holds_alternative<protocol::InputStatePayload>(decoded.payload)) {
    std::cout << "Decoded payload is not InputStatePayload\n";
    return false;
  }

  const auto& decoded_input =
      std::get<protocol::InputStatePayload>(decoded.payload);

  if (decoded_input.command_count != input.command_count ||
      decoded_input.commands[0].input_sequence !=
          input.commands[0].input_sequence ||
      decoded_input.commands[0].buttons != input.commands[0].buttons ||
      decoded_input.commands[0].analog_x != input.commands[0].analog_x ||
      decoded_input.commands[0].analog_y != input.commands[0].analog_y ||
      decoded_input.commands[0].client_time_ms !=
          input.commands[0].client_time_ms) {
    std::cout << "InputStatePayload mismatch in Packet round-trip\n";
    return false;
  }

  return true;
}

}  // namespace

int RunProtocolSerializationTests() {
  int failures = 0;

  if (!RunTest("InputState round-trip", &TestInputStateRoundTrip)) {
    ++failures;
  }
  if (!RunTest("Ping/Pong round-trip", &TestPingPongRoundTrip)) {
    ++failures;
  }
  if (!RunTest("Join messages round-trip", &TestJoinRoundTrip)) {
    ++failures;
  }
  if (!RunTest("PlayerDied round-trip", &TestPlayerDiedRoundTrip)) {
    ++failures;
  }
  if (!RunTest("Command round-trip", &TestCommandRoundTrip)) {
    ++failures;
  }
  if (!RunTest("WorldSnapshot round-trip", &TestWorldSnapshotRoundTrip)) {
    ++failures;
  }
  if (!RunTest("Packet round-trip InputState",
               &TestPacketRoundTripInputState)) {
    ++failures;
  }

  if (failures == 0) {
    std::cout << "All protocol serialization tests passed.\n";
  } else {
    std::cout << failures << " test(s) failed.\n";
  }

  return failures == 0 ? 0 : 1;
}
