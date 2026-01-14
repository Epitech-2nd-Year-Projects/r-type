#include <gtest/gtest.h>

#include <numeric>
#include <string>
#include <vector>

#include "protocol/command.h"
#include "protocol/compression.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"

using namespace protocol;

TEST(CompressionServiceTest, CompressDecompressHello) {
  std::string original = "Hello, World!";
  std::vector<uint8_t> input(original.begin(), original.end());
  std::vector<uint8_t> compressed;
  std::vector<uint8_t> decompressed;

  EXPECT_TRUE(CompressionService::Compress(input, compressed));
  EXPECT_FALSE(compressed.empty());

  EXPECT_TRUE(CompressionService::Decompress(compressed, decompressed));

  std::string result(decompressed.begin(), decompressed.end());
  EXPECT_EQ(input.size(), decompressed.size());
  EXPECT_EQ(original, result);
}

TEST(CompressionServiceTest, CompressDecompressRepeatedData) {
  std::vector<uint8_t> input(1000, 'A');
  std::vector<uint8_t> compressed;
  std::vector<uint8_t> decompressed;

  EXPECT_TRUE(CompressionService::Compress(input, compressed));

  EXPECT_LT(compressed.size(), input.size());

  EXPECT_TRUE(CompressionService::Decompress(compressed, decompressed));
  EXPECT_EQ(input, decompressed);
}

TEST(CompressionServiceTest, EmptyInput) {
  std::vector<uint8_t> input;
  std::vector<uint8_t> compressed;
  std::vector<uint8_t> decompressed;

  EXPECT_TRUE(CompressionService::Compress(input, compressed));
  EXPECT_TRUE(compressed.empty());

  EXPECT_TRUE(CompressionService::Decompress(compressed, decompressed));
  EXPECT_TRUE(decompressed.empty());
}

TEST(CompressionServiceTest, InvalidDecompression) {
  std::vector<uint8_t> garbage = {0x01, 0x02, 0x03};
  std::vector<uint8_t> decompressed;

  EXPECT_FALSE(CompressionService::Decompress(garbage, decompressed));
}

TEST(CompressionServiceTest, PacketIntegration) {
  std::string long_command(200, 'Z');
  CommandPayload cmd_payload;
  cmd_payload.payload = long_command;

  Packet packet;
  packet.header.version = kProtocolVersion;
  packet.header.message_type =
      static_cast<uint8_t>(protocol::message_type::MessageType::kClientCommand);
  packet.header.sequence = 123;
  packet.payload = cmd_payload;

  engine::net::PacketBuffer buffer;
  EXPECT_TRUE(EncodePacket(packet, buffer));

  Header decoded_header;
  engine::net::PacketBuffer read_buffer(buffer.data());
  EXPECT_TRUE(DecodeHeader(read_buffer, decoded_header));

  EXPECT_TRUE(decoded_header.flags & kHeaderFlagCompressed);

  Packet decoded_packet;
  read_buffer.reset_cursor();
  EXPECT_TRUE(DecodePacket(read_buffer, decoded_packet));

  ASSERT_TRUE(std::holds_alternative<CommandPayload>(decoded_packet.payload));
  const auto& decoded_cmd = std::get<CommandPayload>(decoded_packet.payload);
  ASSERT_FALSE(decoded_cmd.payload.empty());
  EXPECT_EQ(decoded_cmd.payload, long_command);
}
