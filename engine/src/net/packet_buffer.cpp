#include "../../include/engine/net/packet_buffer.h"

#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

namespace engine::net {
namespace {

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
constexpr bool kHostIsLittleEndian = true;
#elif defined(_WIN32) || defined(_WIN64)
constexpr bool kHostIsLittleEndian = true;
#else
constexpr bool kHostIsLittleEndian = false;
#endif

template <typename T>
constexpr T ByteSwap(T value) {
  static_assert(std::is_integral_v<T>, "ByteSwap requires integral type");
  using Unsigned = std::make_unsigned_t<T>;
  Unsigned data = static_cast<Unsigned>(value);

  if constexpr (sizeof(T) == 1) {
    return value;
  } else if constexpr (sizeof(T) == 2) {
    data = static_cast<Unsigned>((data >> 8) | (data << 8));
  } else if constexpr (sizeof(T) == 4) {
    data = ((data & 0x000000FFu) << 24) | ((data & 0x0000FF00u) << 8) |
           ((data & 0x00FF0000u) >> 8) | ((data & 0xFF000000u) >> 24);
  } else if constexpr (sizeof(T) == 8) {
    data = ((data & 0x00000000000000FFull) << 56) |
           ((data & 0x000000000000FF00ull) << 40) |
           ((data & 0x0000000000FF0000ull) << 24) |
           ((data & 0x00000000FF000000ull) << 8) |
           ((data & 0x000000FF00000000ull) >> 8) |
           ((data & 0x0000FF0000000000ull) >> 24) |
           ((data & 0x00FF000000000000ull) >> 40) |
           ((data & 0xFF00000000000000ull) >> 56);
  }
  return static_cast<T>(data);
}

}  // namespace

PacketBuffer::PacketBuffer() = default;

PacketBuffer::PacketBuffer(std::size_t reserve_bytes) {
  buffer_.reserve(reserve_bytes);
}

PacketBuffer::PacketBuffer(const void* data, std::size_t size) {
  if (data != nullptr && size > 0) {
    const auto* bytes = static_cast<const value_type*>(data);
    buffer_.assign(bytes, bytes + size);
  }
}

PacketBuffer::PacketBuffer(Storage data)
    : buffer_(std::move(data)), read_offset_(0) {}

void PacketBuffer::clear() {
  buffer_.clear();
  read_offset_ = 0;
}

bool PacketBuffer::seek(std::size_t offset) {
  if (offset > buffer_.size()) return false;
  read_offset_ = offset;
  return true;
}

void PacketBuffer::write_bytes(const void* data, std::size_t size) {
  if (size == 0 || data == nullptr) return;
  const auto* bytes = static_cast<const value_type*>(data);
  buffer_.insert(buffer_.end(), bytes, bytes + size);
}

bool PacketBuffer::read_bytes(void* destination, std::size_t size) {
  if (size == 0) return true;
  if (!EnsureReadable(size)) return false;
  std::memcpy(destination, buffer_.data() + read_offset_, size);
  read_offset_ += size;
  return true;
}

void PacketBuffer::WriteUint8(std::uint8_t value) {
  write_bytes(&value, sizeof(value));
}

void PacketBuffer::WriteUint16(std::uint16_t value) {
  auto network_value = Endian::HostToNetwork(value);
  write_bytes(&network_value, sizeof(network_value));
}

void PacketBuffer::WriteUint32(std::uint32_t value) {
  auto network_value = Endian::HostToNetwork(value);
  write_bytes(&network_value, sizeof(network_value));
}

void PacketBuffer::WriteUint64(std::uint64_t value) {
  auto network_value = Endian::HostToNetwork(value);
  write_bytes(&network_value, sizeof(network_value));
}

void PacketBuffer::WriteInt8(std::int8_t value) {
  auto unsigned_value = static_cast<std::uint8_t>(value);
  WriteUint8(unsigned_value);
}

void PacketBuffer::WriteInt16(std::int16_t value) {
  auto unsigned_value = static_cast<std::uint16_t>(value);
  WriteUint16(unsigned_value);
}

void PacketBuffer::WriteInt32(std::int32_t value) {
  auto unsigned_value = static_cast<std::uint32_t>(value);
  WriteUint32(unsigned_value);
}

void PacketBuffer::WriteInt64(std::int64_t value) {
  auto unsigned_value = static_cast<std::uint64_t>(value);
  WriteUint64(unsigned_value);
}

void PacketBuffer::WriteFloat(float value) {
  static_assert(sizeof(float) == sizeof(std::uint32_t),
                "Unexpected float size");
  std::uint32_t bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  bits = Endian::HostToNetwork(bits);
  write_bytes(&bits, sizeof(bits));
}

void PacketBuffer::WriteDouble(double value) {
  static_assert(sizeof(double) == sizeof(std::uint64_t),
                "Unexpected double size");
  std::uint64_t bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  bits = Endian::HostToNetwork(bits);
  write_bytes(&bits, sizeof(bits));
}

bool PacketBuffer::WriteString(std::string_view value) {
  if (value.size() > std::numeric_limits<std::uint16_t>::max()) return false;
  WriteUint16(static_cast<std::uint16_t>(value.size()));
  if (!value.empty()) write_bytes(value.data(), value.size());
  return true;
}

bool PacketBuffer::ReadUint8(std::uint8_t& value) {
  if (!EnsureReadable(sizeof(value))) return false;
  value = buffer_[read_offset_++];
  return true;
}

bool PacketBuffer::ReadUint16(std::uint16_t& value) {
  std::uint16_t raw = 0;
  if (!read_bytes(&raw, sizeof(raw))) return false;
  value = Endian::NetworkToHost(raw);
  return true;
}

bool PacketBuffer::ReadUint32(std::uint32_t& value) {
  std::uint32_t raw = 0;
  if (!read_bytes(&raw, sizeof(raw))) return false;
  value = Endian::NetworkToHost(raw);
  return true;
}

bool PacketBuffer::ReadUint64(std::uint64_t& value) {
  std::uint64_t raw = 0;
  if (!read_bytes(&raw, sizeof(raw))) return false;
  value = Endian::NetworkToHost(raw);
  return true;
}

bool PacketBuffer::ReadInt8(std::int8_t& value) {
  std::uint8_t raw = 0;
  if (!ReadUint8(raw)) return false;
  value = static_cast<std::int8_t>(raw);
  return true;
}

bool PacketBuffer::ReadInt16(std::int16_t& value) {
  std::uint16_t raw = 0;
  if (!ReadUint16(raw)) return false;
  value = static_cast<std::int16_t>(raw);
  return true;
}

bool PacketBuffer::ReadInt32(std::int32_t& value) {
  std::uint32_t raw = 0;
  if (!ReadUint32(raw)) return false;
  value = static_cast<std::int32_t>(raw);
  return true;
}

bool PacketBuffer::ReadInt64(std::int64_t& value) {
  std::uint64_t raw = 0;
  if (!ReadUint64(raw)) return false;
  value = static_cast<std::int64_t>(raw);
  return true;
}

bool PacketBuffer::ReadFloat(float& value) {
  std::uint32_t bits = 0;
  if (!read_bytes(&bits, sizeof(bits))) return false;
  bits = Endian::NetworkToHost(bits);
  std::memcpy(&value, &bits, sizeof(bits));
  return true;
}

bool PacketBuffer::ReadDouble(double& value) {
  std::uint64_t bits = 0;
  if (!read_bytes(&bits, sizeof(bits))) return false;
  bits = Endian::NetworkToHost(bits);
  std::memcpy(&value, &bits, sizeof(bits));
  return true;
}

bool PacketBuffer::ReadString(std::string& value) {
  std::uint16_t length = 0;
  if (!ReadUint16(length)) return false;
  if (!EnsureReadable(length)) return false;
  value.assign(reinterpret_cast<const char*>(buffer_.data() + read_offset_),
               length);
  read_offset_ += length;
  return true;
}

bool PacketBuffer::EnsureReadable(std::size_t size) const {
  return read_offset_ + size <= buffer_.size();
}

std::uint16_t PacketBuffer::Endian::HostToNetwork(std::uint16_t value) {
  return kHostIsLittleEndian ? ByteSwap(value) : value;
}

std::uint32_t PacketBuffer::Endian::HostToNetwork(std::uint32_t value) {
  return kHostIsLittleEndian ? ByteSwap(value) : value;
}

std::uint64_t PacketBuffer::Endian::HostToNetwork(std::uint64_t value) {
  return kHostIsLittleEndian ? ByteSwap(value) : value;
}

std::uint16_t PacketBuffer::Endian::NetworkToHost(std::uint16_t value) {
  return kHostIsLittleEndian ? ByteSwap(value) : value;
}

std::uint32_t PacketBuffer::Endian::NetworkToHost(std::uint32_t value) {
  return kHostIsLittleEndian ? ByteSwap(value) : value;
}

std::uint64_t PacketBuffer::Endian::NetworkToHost(std::uint64_t value) {
  return kHostIsLittleEndian ? ByteSwap(value) : value;
}

}  // namespace engine::net
