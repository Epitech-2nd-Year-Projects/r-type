#ifndef PROTOCOL_FRAGMENTATION_H_
#define PROTOCOL_FRAGMENTATION_H_

#include <cstdint>
#include <vector>
#include <optional>
#include <unordered_map>
#include <string>

#include "engine/net/packet_buffer.h"
#include "protocol/packet.h"
#include "protocol/header.h"

namespace protocol {

/**
 * @brief Extra header info present when kHeaderFlagFragmented is set.
 */
struct FragmentInfo {
  std::uint16_t fragment_id;    ///< Unique ID of the fragmented message (usually matches sequence).
  std::uint8_t fragment_index;  ///< Index of this fragment (0..count-1).
  std::uint8_t fragment_count;  ///< Total number of fragments.
};

/**
 * @brief Splits a packet into multiple fragments if it exceeds the MTU.
 * @param packet The high-level packet to encode and split.
 * @param mtu The Maximum Transmission Unit (max bytes per datagram).
 * @return A vector of PacketBuffers, each representing a UDP datagram.
 */
std::vector<engine::net::PacketBuffer> SplitPacket(
    const Packet& packet, std::size_t mtu = 1200);

/**
 * @brief Splits an already encoded packet buffer.
 * @param buffer The full encoded packet (Header + Payload).
 * @param mtu The Maximum Transmission Unit.
 * @return A vector of PacketBuffers.
 */
std::vector<engine::net::PacketBuffer> SplitPacketBuffer(
    const engine::net::PacketBuffer& buffer, std::size_t mtu = 1200);

/**
 * @brief Decodes the FragmentInfo from a buffer.
 * @param reader The buffer to read from (must be positioned after Header).
 * @param out_info Output parameter.
 * @return true on success.
 */
bool DecodeFragmentInfo(engine::net::PacketBuffer& reader, FragmentInfo& out_info);

/**
 * @brief Helper class to reassemble fragmented packets.
 */
class FragmentReassembler {
 public:
  struct ReassemblyContext {
    std::uint32_t timestamp_ms;
    std::uint8_t fragments_received;
    std::uint8_t fragment_count;
    std::vector<std::vector<std::uint8_t>> parts;
    std::size_t total_size;
  };

  /**
   * @brief Processes a potential fragment.
   * @param header The decoded packet header.
   * @param reader The buffer positioned after the header.
   * @param from_key A unique key for the sender (e.g. endpoint string).
   * @param now_ms Current timestamp for cleanup.
   * @return std::nullopt if the packet was consumed (partial fragment),
   *         or a PacketBuffer if reassembly is complete (or if not fragmented).
   */
  std::optional<engine::net::PacketBuffer> HandlePacket(
      const Header& header, engine::net::PacketBuffer& reader,
      const std::string& from_key, std::uint32_t now_ms);

  /**
   * @brief Cleans up old incomplete fragments.
   */
  void Cleanup(std::uint32_t now_ms, std::uint32_t timeout_ms = 5000);

 private:
  // Key: sender + fragment_id (which is sequence)
  // Combine sender string and sequence? Or nested map.
  struct FragmentKey {
    std::string sender;
    std::uint16_t fragment_id;

    bool operator==(const FragmentKey& other) const {
      return sender == other.sender && fragment_id == other.fragment_id;
    }
  };

  struct FragmentKeyHash {
    std::size_t operator()(const FragmentKey& k) const {
      return std::hash<std::string>{}(k.sender) ^
             (std::hash<std::uint16_t>{}(k.fragment_id) << 1);
    }
  };

  std::unordered_map<FragmentKey, ReassemblyContext, FragmentKeyHash> pending_;
};

}  // namespace protocol

#endif  // PROTOCOL_FRAGMENTATION_H_
