#ifndef ENGINE_NET_PACKET_BUFFER_H_
#define ENGINE_NET_PACKET_BUFFER_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace engine::net {

/**
 * @brief Small growable byte buffer for encoding/decoding packets
 *
 * Provides sequential read/write helpers for fixed-size integers, floating
 * point values and length-prefixed strings. All numeric primitives are written
 * in network byte order to guarantee consistent wire representation across
 * hosts. Strings are stored as a uint16 length followed by raw bytes, making
 * the class usable for simple protocol serialization tasks.
 */
class PacketBuffer {
 public:
  using value_type = std::uint8_t;
  using Storage = std::vector<value_type>;
  using ConstSpan = std::span<const value_type>;
  using Span = std::span<value_type>;

  /**
   * @brief Static endian conversion helpers
   */
  struct Endian {
    static std::uint16_t HostToNetwork(std::uint16_t value);
    static std::uint32_t HostToNetwork(std::uint32_t value);
    static std::uint64_t HostToNetwork(std::uint64_t value);

    static std::uint16_t NetworkToHost(std::uint16_t value);
    static std::uint32_t NetworkToHost(std::uint32_t value);
    static std::uint64_t NetworkToHost(std::uint64_t value);
  };

  PacketBuffer();
  explicit PacketBuffer(std::size_t reserve_bytes);
  explicit PacketBuffer(ConstSpan data);
  explicit PacketBuffer(Storage data);

  /**
   * @brief Direct access to raw storage
   */
  const Storage& storage() const noexcept { return buffer_; }
  Storage& storage() noexcept { return buffer_; }

  /**
   * @brief Span helpers for networking APIs
   */
  ConstSpan data() const noexcept { return ConstSpan(buffer_); }
  Span data() noexcept { return Span(buffer_); }

  /**
   * @brief Buffer size and cursor helpers
   */
  std::size_t size() const noexcept { return buffer_.size(); }
  bool empty() const noexcept { return buffer_.empty(); }
  std::size_t read_offset() const noexcept { return read_offset_; }
  std::size_t remaining() const noexcept {
    return buffer_.size() - read_offset_;
  }

  /**
   * @brief Reset buffer content and cursor
   */
  void clear();

  /**
   * @brief Reset read cursor to the beginning
   */
  void reset_cursor() noexcept { read_offset_ = 0; }

  /**
   * @brief Move read cursor to explicit offset
   */
  bool seek(std::size_t offset);

  /**
   * @brief Reserve storage to minimize reallocations
   */
  void reserve(std::size_t capacity) { buffer_.reserve(capacity); }

  /**
   * @brief Copy raw bytes into the buffer
   */
  void write_bytes(ConstSpan data);
  void write_bytes(std::span<const std::byte> data);
  template <std::size_t N>
  void write_bytes(const std::array<value_type, N>& data) {
    write_bytes(ConstSpan(data));
  }

  /**
   * @brief Read raw bytes from the buffer
   */
  bool read_bytes(Span destination);
  bool read_bytes(std::span<std::byte> destination);
  template <std::size_t N>
  bool read_bytes(std::array<value_type, N>& destination) {
    return read_bytes(Span(destination));
  }

  /**
   * @name Integer write helpers (network byte order)
   */
  ///@{
  void WriteUint8(std::uint8_t value);
  void WriteUint16(std::uint16_t value);
  void WriteUint32(std::uint32_t value);
  void WriteUint64(std::uint64_t value);

  void WriteInt8(std::int8_t value);
  void WriteInt16(std::int16_t value);
  void WriteInt32(std::int32_t value);
  void WriteInt64(std::int64_t value);
  ///@}

  /**
   * @name Floating point write helpers (IEEE754 via network byte order)
   */
  ///@{
  void WriteFloat(float value);
  void WriteDouble(double value);
  ///@}

  /**
   * @brief Write length-prefixed string (uint16 length)
   * @return false if string exceeds maximum encodable length
   */
  bool WriteString(std::string_view value);

  /**
   * @name Integer read helpers (network byte order)
   */
  ///@{
  bool ReadUint8(std::uint8_t& value);
  bool ReadUint16(std::uint16_t& value);
  bool ReadUint32(std::uint32_t& value);
  bool ReadUint64(std::uint64_t& value);

  bool ReadInt8(std::int8_t& value);
  bool ReadInt16(std::int16_t& value);
  bool ReadInt32(std::int32_t& value);
  bool ReadInt64(std::int64_t& value);
  ///@}

  /**
   * @name Floating point read helpers
   */
  ///@{
  bool ReadFloat(float& value);
  bool ReadDouble(double& value);
  ///@}

  /**
   * @brief Read length-prefixed string
   */
  bool ReadString(std::string& value);

 private:
  bool EnsureReadable(std::size_t size) const;

  Storage buffer_;
  std::size_t read_offset_{0};
};

}  // namespace engine::net

#endif  // ENGINE_NET_PACKET_BUFFER_H_
