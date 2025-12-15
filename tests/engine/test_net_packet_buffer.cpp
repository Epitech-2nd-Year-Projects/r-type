#include <gtest/gtest.h>

#include "engine/net/packet_buffer.h"

TEST(PacketBufferTest, WriteAndReadPrimitives) {
  engine::net::PacketBuffer buffer;

  int32_t i = 123;
  float f = 3.14f;
  bool b = true;

  buffer.WriteInt32(i);
  buffer.WriteFloat(f);
  buffer.WriteUint8(b ? 1 : 0);

  int32_t ri;
  float rf;
  uint8_t rb_int;

  EXPECT_TRUE(buffer.ReadInt32(ri));
  EXPECT_TRUE(buffer.ReadFloat(rf));
  EXPECT_TRUE(buffer.ReadUint8(rb_int));

  EXPECT_EQ(ri, i);
  EXPECT_FLOAT_EQ(rf, f);
  EXPECT_EQ(rb_int, 1);
}

TEST(PacketBufferTest, VectorSerialization) {
  engine::net::PacketBuffer buffer;
  std::vector<int32_t> numbers = {1, 2, 3, 4, 5};

  buffer.WriteUint32(static_cast<uint32_t>(numbers.size()));
  for (int32_t num : numbers) {
    buffer.WriteInt32(num);
  }

  uint32_t count;
  ASSERT_TRUE(buffer.ReadUint32(count));
  EXPECT_EQ(count, numbers.size());

  std::vector<int32_t> read_numbers;
  read_numbers.reserve(count);
  for (uint32_t k = 0; k < count; ++k) {
    int32_t val;
    ASSERT_TRUE(buffer.ReadInt32(val));
    read_numbers.push_back(val);
  }

  EXPECT_EQ(read_numbers, numbers);
}

TEST(PacketBufferTest, StringSerialization) {
  engine::net::PacketBuffer buffer;
  std::string s = "Hello, World!";

  EXPECT_TRUE(buffer.WriteString(s));

  std::string rs;
  EXPECT_TRUE(buffer.ReadString(rs));

  EXPECT_EQ(s, rs);
}

TEST(PacketBufferTest, ClearResetsBuffer) {
  engine::net::PacketBuffer buffer;
  buffer.WriteInt32(42);

  EXPECT_GT(buffer.size(), 0);

  buffer.clear();
  EXPECT_EQ(buffer.size(), 0);
  EXPECT_TRUE(buffer.empty());
}
