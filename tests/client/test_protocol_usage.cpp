#include <iostream>
#include <vector>
#include <cstdint>
#include <string>

#include "input_layer.h"
#include "engine/input.h"
#include "protocol/input_state.h"
#include "protocol/world_snapshot.h"
#include "engine/net/packet_buffer.h"

template <typename F>
bool RunTest(const char* name, F&& fn) {
    std::cout << "Running " << name << "... ";
    const bool ok = fn();
    std::cout << (ok ? "[OK]" : "[FAIL]") << "\n";
    return ok;
}

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

bool TestClientInputToProtocol() {
    std::cout << "\n  [Step 1] Initializing Input System...\n";
    engine::input::InputManager input_manager;
    client::InputLayer input_layer(input_manager);
    input_layer.ApplyDefaultBindings();

    std::cout << "  [Step 2] Simulating user pressing 'W'...\n";
    input_manager.HandleKey(engine::input::Key::kW, true);
    input_layer.Update();

    if (input_layer.state().move_up) {
        std::cout << "    -> Success: InputLayer correctly mapped 'W' to 'MoveUp' action.\n";
    } else {
        std::cerr << "    -> Error: InputLayer failed to map 'W' to MoveUp\n";
        return false;
    }

    std::cout << "  [Step 3] Converting Client ActionState to Protocol Payload...\n";
    protocol::InputStatePayload payload = CreatePayloadFromState(input_layer.state(), 101);

    if (payload.commands[0].buttons & protocol::kInputUp) {
        std::cout << "    -> Success: Payload bitmask contains kInputUp.\n";
    } else {
        std::cerr << "    -> Error: Payload failed to include kInputUp bit\n";
        return false;
    }

    std::cout << "  [Step 4] Encoding payload to binary packet...\n";
    engine::net::PacketBuffer buffer;
    if (!protocol::EncodeInputState(payload, buffer)) {
        std::cerr << "    -> Error: Failed to encode input state\n";
        return false;
    }
    std::cout << "    -> Encoded " << buffer.size() << " bytes.\n";

    std::cout << "  [Step 5] Decoding binary packet back to struct (Round-trip verification)...\n";
    engine::net::PacketBuffer read_buffer(buffer.storage());
    protocol::InputStatePayload decoded{};
    if (!protocol::DecodeInputState(read_buffer, decoded)) {
        std::cerr << "    -> Error: Failed to decode\n";
        return false;
    }

    if (decoded.commands[0].buttons == protocol::kInputUp) {
        std::cout << "    -> Success: Decoded buttons match original input.\n";
    } else {
        std::cerr << "    -> Error: Decoded buttons mismatch\n";
        return false;
    }

    return true;
}

bool TestSnapshotIntegration() {
    std::cout << "\n  [Step 1] Creating a mock Server Snapshot...\n";
    protocol::WorldSnapshotPayload server_snapshot{};
    server_snapshot.snapshot_id = 50;
    
    protocol::EntityDelta delta{};
    delta.op = protocol::EntityDeltaOp::kCreate;
    delta.entity_id = 999;
    delta.state.type = 2;
    delta.state.x = 800;
    delta.state.y = 100;
    server_snapshot.deltas.push_back(delta);
    
    std::cout << "    -> Added EntityCreate Delta: ID=999, Type=2, Pos=(800, 100)\n";

    std::cout << "  [Step 2] Encoding Snapshot to binary...\n";
    engine::net::PacketBuffer buffer;
    if (!protocol::EncodeWorldSnapshot(server_snapshot, buffer)) {
        std::cerr << "    -> Error: Encode failed\n";
        return false;
    }
    std::cout << "    -> Encoded " << buffer.size() << " bytes.\n";

    std::cout << "  [Step 3] Client decoding binary packet...\n";
    engine::net::PacketBuffer read_buffer(buffer.storage());
    protocol::WorldSnapshotPayload decoded{};
    if (!protocol::DecodeWorldSnapshot(read_buffer, decoded)) {
        std::cerr << "    -> Error: Decode failed\n";
        return false;
    }

    std::cout << "  [Step 4] Verifying decoded data integrity...\n";
    bool entity_found = false;
    for (const auto& d : decoded.deltas) {
        if (d.entity_id == 999 && d.op == protocol::EntityDeltaOp::kCreate) {
            std::cout << "    -> Found Entity Delta: ID=" << d.entity_id << "\n";
            if (d.state.x == 800 && d.state.y == 100) {
                entity_found = true;
                std::cout << "    -> Position matches (800, 100).\n";
            }
        }
    }

    if (!entity_found) {
        std::cerr << "    -> Error: Client logic would not have found the correct entity update\n";
        return false;
    }

    return true;
}

int main() {
    int failures = 0;
    if (!RunTest("TestClientInputToProtocol", TestClientInputToProtocol)) failures++;
    if (!RunTest("TestSnapshotIntegration", TestSnapshotIntegration)) failures++;
    
    return failures == 0 ? 0 : 1;
}
