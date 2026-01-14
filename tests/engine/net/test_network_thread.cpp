#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "engine/game_runtime.h"
#include "engine/net/events.h"
#include "engine/net/udp_socket.h"
#include "protocol/packet.h"
#include "protocol/ping.h"

using namespace engine;

TEST(GameRuntimeNetworkTest, QueuesIncomingPackets) {
  GameRuntime runtime;
  std::atomic<bool> received{false};
  runtime.EventBus().Subscribe<net::PacketReceivedEvent>(
      [&](const net::PacketReceivedEvent& event) {
        net::PacketBuffer buffer(event.packet.payload);
        protocol::Packet decoded_packet;
        if (protocol::DecodePacket(buffer, decoded_packet)) {
          if (std::holds_alternative<protocol::PingPayload>(
                  decoded_packet.payload)) {
            received = true;
          }
        }
      });

  runtime.StartNetwork(0);
  runtime.Start();

  std::uint16_t test_port = runtime.GetBoundPort();
  ASSERT_NE(test_port, 0);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  net::UdpSocket sender;
  sender.open(net::UdpSocket::Protocol::kIpv4);

  protocol::Packet packet;
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(protocol::message_type::MessageType::kPing);
  packet.payload = protocol::PingPayload{12345};

  net::PacketBuffer buffer;
  protocol::EncodePacket(packet, buffer);

  sender.send_to(buffer.data(), net::Endpoint::LoopbackIpv4(test_port));

  for (int i = 0; i < 20; ++i) {
    if (received) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  runtime.Stop();
  EXPECT_TRUE(received);
}
