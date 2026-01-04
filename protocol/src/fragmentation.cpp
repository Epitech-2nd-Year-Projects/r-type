#include "protocol/fragmentation.h"

#include <algorithm>
#include <cstring>
#include <iterator>

namespace protocol {

std::vector<engine::net::PacketBuffer> SplitPacket(const Packet& packet,
                                                   std::size_t mtu) {
  std::vector<engine::net::PacketBuffer> result;

  engine::net::PacketBuffer message_buffer;
  message_buffer.reserve(4096);
  if (!EncodePacket(packet, message_buffer)) {
    return result;
  }
  return SplitPacketBuffer(message_buffer, mtu);
}

std::vector<engine::net::PacketBuffer> SplitPacketBuffer(
    const engine::net::PacketBuffer& buffer, std::size_t mtu) {
  std::vector<engine::net::PacketBuffer> result;

  const std::size_t total_size = buffer.size();

  engine::net::PacketBuffer reader = buffer;
  Header header;
  if (!DecodeHeader(reader, header)) {
    return result;
  }

  engine::net::PacketBuffer dummy;
  EncodeHeader(header, dummy);
  const std::size_t header_size = dummy.size();
  const std::size_t frag_info_size =
      sizeof(std::uint16_t) + sizeof(std::uint8_t) * 2;
  const std::size_t overhead = header_size + frag_info_size;

  if (mtu <= overhead) {
    return result;
  }

  const std::size_t max_chunk_size = mtu - overhead;

  if (total_size <= mtu) {
      result.push_back(buffer);
      return result;
  }

  const std::size_t count = (total_size + max_chunk_size - 1) / max_chunk_size;
  if (count > 255) {
    return result;
  }

  const std::uint8_t fragment_count = static_cast<std::uint8_t>(count);
  const std::uint16_t fragment_id = static_cast<std::uint16_t>(header.sequence);

  for (std::uint8_t i = 0; i < fragment_count; ++i) {
    std::size_t offset = i * max_chunk_size;
    std::size_t remaining = total_size - offset;
    std::size_t chunk_size = std::min(remaining, max_chunk_size);

    engine::net::PacketBuffer frag_buf;
    frag_buf.reserve(mtu);

    Header frag_header = header;
    frag_header.flags |= kHeaderFlagFragmented;
    EncodeHeader(frag_header, frag_buf);

    frag_buf.WriteUint16(fragment_id);
    frag_buf.WriteUint8(i);
    frag_buf.WriteUint8(fragment_count);

    const std::vector<std::uint8_t>& raw = buffer.storage();
    frag_buf.write_bytes(
        std::span<const std::uint8_t>(raw.data() + offset, chunk_size));

    result.push_back(std::move(frag_buf));
  }

  return result;
}

bool DecodeFragmentInfo(engine::net::PacketBuffer& reader,
                        FragmentInfo& out_info) {
  if (!reader.ReadUint16(out_info.fragment_id) ||
      !reader.ReadUint8(out_info.fragment_index) ||
      !reader.ReadUint8(out_info.fragment_count)) {
    return false;
  }
  return true;
}

std::optional<engine::net::PacketBuffer> FragmentReassembler::HandlePacket(
    const Header& header, engine::net::PacketBuffer& reader,
    const std::string& from_key, std::uint32_t now_ms) {
  if (!(header.flags & kHeaderFlagFragmented)) {
    engine::net::PacketBuffer full;
    full.reserve(20 + reader.remaining());
    EncodeHeader(header, full);

    std::vector<std::uint8_t> remaining_bytes(reader.remaining());
    reader.read_bytes(remaining_bytes);
    full.write_bytes(remaining_bytes);

    return full;
  }

  FragmentInfo info;
  if (!DecodeFragmentInfo(reader, info)) {
    return std::nullopt;
  }

  FragmentKey key{from_key, info.fragment_id};
  auto& ctx = pending_[key];

  if (ctx.parts.empty()) {
    ctx.parts.resize(info.fragment_count);
    ctx.fragment_count = info.fragment_count;
    ctx.timestamp_ms = now_ms;
    ctx.fragments_received = 0;
    ctx.total_size = 0;
  } else if (ctx.fragment_count != info.fragment_count) {
    return std::nullopt;
  }

  if (info.fragment_index >= ctx.parts.size()) {
    return std::nullopt;
  }

  if (ctx.parts[info.fragment_index].empty()) {
    std::vector<std::uint8_t> chunk(reader.remaining());
    reader.read_bytes(chunk);

    ctx.total_size += chunk.size();
    ctx.parts[info.fragment_index] = std::move(chunk);
    ctx.fragments_received++;
    ctx.timestamp_ms = now_ms;
  }

  if (ctx.fragments_received == ctx.fragment_count) {
    engine::net::PacketBuffer reassembled;
    reassembled.reserve(ctx.total_size);

    for (const auto& part : ctx.parts) {
      reassembled.write_bytes(part);
    }

    pending_.erase(key);
    return reassembled;
  }

  return std::nullopt;
}

void FragmentReassembler::Cleanup(std::uint32_t now_ms,
                                  std::uint32_t timeout_ms) {
  for (auto it = pending_.begin(); it != pending_.end();) {
    if (now_ms - it->second.timestamp_ms > timeout_ms) {
      it = pending_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace protocol
