#include "protocol/compression.h"

#include <lz4.h>

#include <cstring>
#include <limits>

#include "engine/net/packet_buffer.h"

namespace protocol {

bool CompressionService::Compress(std::span<const std::uint8_t> input,
                                  std::vector<std::uint8_t>& output) {
  if (input.empty()) {
    output.clear();
    return true;
  }

  if (input.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
    return false;
  }

  const int input_size = static_cast<int>(input.size());
  const int max_dst_size = LZ4_compressBound(input_size);
  output.resize(sizeof(std::uint32_t) + max_dst_size);

  std::uint32_t size_header = engine::net::PacketBuffer::Endian::HostToNetwork(
      static_cast<std::uint32_t>(input_size));
  std::memcpy(output.data(), &size_header, sizeof(std::uint32_t));

  const int compressed_bytes = LZ4_compress_default(
      reinterpret_cast<const char*>(input.data()),
      reinterpret_cast<char*>(output.data() + sizeof(std::uint32_t)),
      input_size, max_dst_size);

  if (compressed_bytes <= 0) {
    return false;
  }

  output.resize(sizeof(std::uint32_t) + compressed_bytes);
  return true;
}

bool CompressionService::Decompress(std::span<const std::uint8_t> input,
                                    std::vector<std::uint8_t>& output) {
  if (input.empty()) {
    output.clear();
    return true;
  }

  if (input.size() < sizeof(std::uint32_t)) {
    return false;
  }

  std::uint32_t net_size_header = 0;
  std::memcpy(&net_size_header, input.data(), sizeof(std::uint32_t));
  std::uint32_t uncompressed_size =
      engine::net::PacketBuffer::Endian::NetworkToHost(net_size_header);

  if (uncompressed_size == 0) {
    output.clear();
    return true;
  }

  if (uncompressed_size >
      static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
    return false;
  }

  output.resize(uncompressed_size);

  const int bytes_processed = LZ4_decompress_safe(
      reinterpret_cast<const char*>(input.data() + sizeof(std::uint32_t)),
      reinterpret_cast<char*>(output.data()),
      static_cast<int>(input.size() - sizeof(std::uint32_t)),
      static_cast<int>(uncompressed_size));

  if (bytes_processed < 0 ||
      static_cast<std::uint32_t>(bytes_processed) != uncompressed_size) {
    return false;
  }

  return true;
}

int CompressionService::GetMaxCompressedSize(int input_size) {
  return LZ4_compressBound(input_size);
}

}  // namespace protocol
