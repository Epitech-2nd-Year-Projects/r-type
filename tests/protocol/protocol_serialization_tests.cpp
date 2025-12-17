#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
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

TEST(ProtocolSerializationTests, InputStateRoundTrip) {
  engine::net::PacketBuffer buffer;

  protocol::InputStatePayload original{};
  original.command_count = 1;
  original.commands[0].input_sequence = 42u;
  original.commands[0].buttons = protocol::kInputUp | protocol::kInputFire;
  original.commands[0].analog_x = 123;
  original.commands[0].analog_y = -45;
  original.commands[0].client_time_ms = 1337u;

  ASSERT_TRUE(protocol::EncodeInputState(original, buffer));

  engine::net::PacketBuffer read_buffer(buffer.storage());
  protocol::InputStatePayload decoded{};

  ASSERT_TRUE(protocol::DecodeInputState(read_buffer, decoded));
  EXPECT_EQ(decoded.command_count, original.command_count);

  EXPECT_EQ(decoded.commands[0].input_sequence,
            original.commands[0].input_sequence);
  EXPECT_EQ(decoded.commands[0].buttons, original.commands[0].buttons);
  EXPECT_EQ(decoded.commands[0].analog_x, original.commands[0].analog_x);
  EXPECT_EQ(decoded.commands[0].analog_y, original.commands[0].analog_y);
  EXPECT_EQ(decoded.commands[0].client_time_ms,
            original.commands[0].client_time_ms);
}

TEST(ProtocolSerializationTests, PingPongRoundTrip) {
  engine::net::PacketBuffer buffer;

  protocol::PingPayload ping{};
  ping.client_time_ms = 555u;

  ASSERT_TRUE(protocol::EncodePing(ping, buffer));

  engine::net::PacketBuffer read_ping(buffer.storage());
  protocol::PingPayload decoded_ping{};
  ASSERT_TRUE(protocol::DecodePing(read_ping, decoded_ping));
  EXPECT_EQ(decoded_ping.client_time_ms, ping.client_time_ms);

  engine::net::PacketBuffer buffer2;
  protocol::PongPayload pong{};
  pong.client_time_ms = 555u;
  pong.server_time_ms = 999u;

  ASSERT_TRUE(protocol::EncodePong(pong, buffer2));

  engine::net::PacketBuffer read_pong(buffer2.storage());
  protocol::PongPayload decoded_pong{};
  ASSERT_TRUE(protocol::DecodePong(read_pong, decoded_pong));

  EXPECT_EQ(decoded_pong.client_time_ms, pong.client_time_ms);
  EXPECT_EQ(decoded_pong.server_time_ms, pong.server_time_ms);
}

TEST(ProtocolSerializationTests, JoinRoundTrip) {
  {
    engine::net::PacketBuffer buffer;

    protocol::JoinRequestPayload original{};
    original.client_version = 1;
    original.player_name = "Lindon";
    original.room_code = "ROOM42";

    ASSERT_TRUE(protocol::EncodeJoinRequest(original, buffer));

    engine::net::PacketBuffer read_buffer(buffer.storage());
    protocol::JoinRequestPayload decoded{};
    ASSERT_TRUE(protocol::DecodeJoinRequest(read_buffer, decoded));

    EXPECT_EQ(decoded.client_version, original.client_version);
    EXPECT_EQ(decoded.player_name, original.player_name);
    EXPECT_EQ(decoded.room_code, original.room_code);
  }

  {
    engine::net::PacketBuffer buffer;

    protocol::JoinAcceptPayload original{};
    original.server_version = 1;
    original.player_id = 123u;
    original.max_players = 4u;
    original.tick_rate = 60u;
    original.seed = 0xDEADBEEFu;

    ASSERT_TRUE(protocol::EncodeJoinAccept(original, buffer));

    engine::net::PacketBuffer read_buffer(buffer.storage());
    protocol::JoinAcceptPayload decoded{};
    ASSERT_TRUE(protocol::DecodeJoinAccept(read_buffer, decoded));

    EXPECT_EQ(decoded.server_version, original.server_version);
    EXPECT_EQ(decoded.player_id, original.player_id);
    EXPECT_EQ(decoded.max_players, original.max_players);
    EXPECT_EQ(decoded.tick_rate, original.tick_rate);
    EXPECT_EQ(decoded.seed, original.seed);
  }

  {
    engine::net::PacketBuffer buffer;

    protocol::JoinRejectPayload original{};
    original.server_version = 1;
    original.reason = protocol::JoinRejectReason::kServerFull;
    original.message = "Server is full";

    ASSERT_TRUE(protocol::EncodeJoinReject(original, buffer));

    engine::net::PacketBuffer read_buffer(buffer.storage());
    protocol::JoinRejectPayload decoded{};
    ASSERT_TRUE(protocol::DecodeJoinReject(read_buffer, decoded));

    EXPECT_EQ(decoded.server_version, original.server_version);
    EXPECT_EQ(decoded.reason, original.reason);
    EXPECT_EQ(decoded.message, original.message);
  }
}

TEST(ProtocolSerializationTests, PlayerDiedRoundTrip) {
  engine::net::PacketBuffer buffer;

  protocol::PlayerDiedPayload original{};
  original.player_id = 7u;
  original.killer_entity_id = 999u;
  original.cause = protocol::DeathCause::kProjectile;
  original.remaining_lives = 2u;

  ASSERT_TRUE(protocol::EncodePlayerDied(original, buffer));

  engine::net::PacketBuffer read_buffer(buffer.storage());
  protocol::PlayerDiedPayload decoded{};
  ASSERT_TRUE(protocol::DecodePlayerDied(read_buffer, decoded));

  EXPECT_EQ(decoded.player_id, original.player_id);
  EXPECT_EQ(decoded.killer_entity_id, original.killer_entity_id);
  EXPECT_EQ(decoded.cause, original.cause);
  EXPECT_EQ(decoded.remaining_lives, original.remaining_lives);
}

TEST(ProtocolSerializationTests, CommandRoundTrip) {
  engine::net::PacketBuffer buffer;

  protocol::CommandPayload original{};
  original.command_id =
      static_cast<std::uint16_t>(protocol::CommandType::kChatMessage);
  original.payload = R"({"text":"hello world"})";

  ASSERT_TRUE(protocol::EncodeCommand(original, buffer));

  engine::net::PacketBuffer read_buffer(buffer.storage());
  protocol::CommandPayload decoded{};
  ASSERT_TRUE(protocol::DecodeCommand(read_buffer, decoded));

  EXPECT_EQ(decoded.command_id, original.command_id);
  EXPECT_EQ(decoded.payload, original.payload);
}

TEST(ProtocolSerializationTests, WorldSnapshotRoundTrip) {
  engine::net::PacketBuffer buffer;

  protocol::WorldSnapshotPayload original{};
  original.snapshot_id = 10u;
  original.base_snapshot_id = protocol::kNoBaseSnapshotId;
  original.server_tick = 1234u;
   original.current_wave = 5u;

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
    d.state.score = 77u;
    d.state.lives = 4u;
    d.state.player_id = 9u;
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

  ASSERT_TRUE(protocol::EncodeWorldSnapshot(original, buffer));

  engine::net::PacketBuffer read_buffer(buffer.storage());
  protocol::WorldSnapshotPayload decoded{};
  ASSERT_TRUE(protocol::DecodeWorldSnapshot(read_buffer, decoded));

  EXPECT_EQ(decoded.snapshot_id, original.snapshot_id);
  EXPECT_EQ(decoded.base_snapshot_id, original.base_snapshot_id);
  EXPECT_EQ(decoded.server_tick, original.server_tick);
  EXPECT_EQ(decoded.current_wave, original.current_wave);
  EXPECT_EQ(decoded.deltas.size(), original.deltas.size());

  ASSERT_EQ(decoded.deltas.size(), 3u);

  {
    const protocol::EntityDelta& src = original.deltas[0];
    const protocol::EntityDelta& dst = decoded.deltas[0];

    EXPECT_EQ(dst.op, protocol::EntityDeltaOp::kCreate);
    EXPECT_EQ(dst.entity_id, src.entity_id);

    EXPECT_EQ(dst.state.entity_id, src.state.entity_id);
    EXPECT_EQ(dst.state.type, src.state.type);
    EXPECT_EQ(dst.state.x, src.state.x);
    EXPECT_EQ(dst.state.y, src.state.y);
    EXPECT_EQ(dst.state.vx, src.state.vx);
    EXPECT_EQ(dst.state.vy, src.state.vy);
    EXPECT_EQ(dst.state.hp, src.state.hp);
    EXPECT_EQ(dst.state.flags, src.state.flags);
    EXPECT_EQ(dst.state.score, src.state.score);
    EXPECT_EQ(dst.state.lives, src.state.lives);
    EXPECT_EQ(dst.state.player_id, src.state.player_id);

    EXPECT_EQ(dst.field_mask, 0u);
  }

  {
    const protocol::EntityDelta& src = original.deltas[1];
    const protocol::EntityDelta& dst = decoded.deltas[1];

    EXPECT_EQ(dst.op, protocol::EntityDeltaOp::kUpdate);
    EXPECT_EQ(dst.entity_id, src.entity_id);

    EXPECT_EQ(dst.field_mask, src.field_mask);

    EXPECT_TRUE(dst.field_mask & protocol::kFieldX);
    EXPECT_EQ(dst.state.x, src.state.x);

    EXPECT_TRUE(dst.field_mask & protocol::kFieldHp);
    EXPECT_EQ(dst.state.hp, src.state.hp);

    EXPECT_EQ(dst.state.type, 0);
    EXPECT_EQ(dst.state.y, 0);
    EXPECT_EQ(dst.state.vx, 0);
    EXPECT_EQ(dst.state.vy, 0);
    EXPECT_EQ(dst.state.flags, 0);
    EXPECT_EQ(dst.state.score, 0u);
    EXPECT_EQ(dst.state.lives, 0u);
    EXPECT_EQ(dst.state.player_id, 0u);
  }

  {
    const protocol::EntityDelta& src = original.deltas[2];
    const protocol::EntityDelta& dst = decoded.deltas[2];

    EXPECT_EQ(dst.op, protocol::EntityDeltaOp::kDelete);
    EXPECT_EQ(dst.entity_id, src.entity_id);

    EXPECT_EQ(dst.field_mask, 0u);
    EXPECT_EQ(dst.state.entity_id, src.entity_id);

    EXPECT_EQ(dst.state.type, 0);
    EXPECT_EQ(dst.state.x, 0);
    EXPECT_EQ(dst.state.y, 0);
    EXPECT_EQ(dst.state.vx, 0);
    EXPECT_EQ(dst.state.vy, 0);
    EXPECT_EQ(dst.state.hp, 0);
    EXPECT_EQ(dst.state.flags, 0);
    EXPECT_EQ(dst.state.score, 0u);
    EXPECT_EQ(dst.state.lives, 0u);
    EXPECT_EQ(dst.state.player_id, 0u);
  }
}

TEST(ProtocolSerializationTests, PacketRoundTripInputState) {
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
  ASSERT_TRUE(protocol::EncodePacket(original, buffer))
      << "EncodePacket(InputState) returned false";

  engine::net::PacketBuffer read_buffer(buffer.storage());
  protocol::Packet decoded{};
  ASSERT_TRUE(protocol::DecodePacket(read_buffer, decoded))
      << "DecodePacket(InputState) returned false";

  EXPECT_EQ(decoded.header.version, original.header.version);
  EXPECT_EQ(decoded.header.message_type, original.header.message_type);
  EXPECT_EQ(decoded.header.flags, original.header.flags);
  EXPECT_EQ(decoded.header.sequence, original.header.sequence);
  EXPECT_EQ(decoded.header.ack, original.header.ack);
  EXPECT_EQ(decoded.header.ack_bits, original.header.ack_bits);
  EXPECT_EQ(decoded.header.timestamp_ms, original.header.timestamp_ms);

  ASSERT_TRUE(
      std::holds_alternative<protocol::InputStatePayload>(decoded.payload))
      << "Decoded payload is not InputStatePayload";

  const auto& decoded_input =
      std::get<protocol::InputStatePayload>(decoded.payload);

  EXPECT_EQ(decoded_input.command_count, input.command_count);
  EXPECT_EQ(decoded_input.commands[0].input_sequence,
            input.commands[0].input_sequence);
  EXPECT_EQ(decoded_input.commands[0].buttons, input.commands[0].buttons);
  EXPECT_EQ(decoded_input.commands[0].analog_x, input.commands[0].analog_x);
  EXPECT_EQ(decoded_input.commands[0].analog_y, input.commands[0].analog_y);
  EXPECT_EQ(decoded_input.commands[0].client_time_ms,
            input.commands[0].client_time_ms);
}

}  // namespace
