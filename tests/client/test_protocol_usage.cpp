#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <string>

#include "input_layer.h"
#include "engine/input.h"
#include "protocol/input_state.h"
#include "protocol/world_snapshot.h"
#include "engine/net/packet_buffer.h"

protocol::InputStatePayload CreatePayloadFromState(const client::ActionState& state, uint32_t sequence) {
    protocol::InputStatePayload payload{};
    payload.command_count = 1;
    auto& cmd = payload.commands[0];
    cmd.input_sequence = sequence;
    cmd.client_time_ms = 0;
    
    if (state.move_up) cmd.buttons |= protocol::kInputUp;
    if (state.move_down) cmd.buttons |= protocol::kInputDown;
    if (state.move_left) cmd.buttons |= protocol::kInputLeft;
    if (state.move_right) cmd.buttons |= protocol::kInputRight;
    if (state.shoot) cmd.buttons |= protocol::kInputFire;
    
    return payload;
}

TEST(ProtocolUsageTest, ClientInputToProtocol) {
    engine::input::InputManager input_manager;
    client::InputLayer input_layer(input_manager);
    input_layer.ApplyDefaultBindings();

    input_manager.HandleKey(engine::input::Key::kZ, true);
    input_layer.Update();

    EXPECT_TRUE(input_layer.state().move_up) << "InputLayer failed to map 'Z' to MoveUp";

    protocol::InputStatePayload payload = CreatePayloadFromState(input_layer.state(), 101);

    EXPECT_TRUE(payload.commands[0].buttons & protocol::kInputUp) << "Payload failed to include kInputUp bit";

    engine::net::PacketBuffer buffer;
    ASSERT_TRUE(protocol::EncodeInputState(payload, buffer)) << "Failed to encode input state";
    
    engine::net::PacketBuffer read_buffer(buffer.storage());
    protocol::InputStatePayload decoded{};
    ASSERT_TRUE(protocol::DecodeInputState(read_buffer, decoded)) << "Failed to decode";

    EXPECT_EQ(decoded.commands[0].buttons, protocol::kInputUp) << "Decoded buttons mismatch";
}

TEST(ProtocolUsageTest, SnapshotIntegration) {
    protocol::WorldSnapshotPayload server_snapshot{};
    server_snapshot.snapshot_id = 50;
    
    protocol::EntityDelta delta{};
    delta.op = protocol::EntityDeltaOp::kCreate;
    delta.entity_id = 999;
    delta.state.type = 2;
    delta.state.x = 800;
    delta.state.y = 100;
    server_snapshot.deltas.push_back(delta);
    
    engine::net::PacketBuffer buffer;
    ASSERT_TRUE(protocol::EncodeWorldSnapshot(server_snapshot, buffer)) << "Encode failed";

    engine::net::PacketBuffer read_buffer(buffer.storage());
    protocol::WorldSnapshotPayload decoded{};
    ASSERT_TRUE(protocol::DecodeWorldSnapshot(read_buffer, decoded)) << "Decode failed";
    bool entity_found = false;
    for (const auto& d : decoded.deltas) {
        if (d.entity_id == 999 && d.op == protocol::EntityDeltaOp::kCreate) {
            EXPECT_EQ(d.state.x, 800);
            EXPECT_EQ(d.state.y, 100);
            entity_found = true;
        }
    }

    EXPECT_TRUE(entity_found) << "Client logic would not have found the correct entity update";
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
