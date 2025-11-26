# R-Type UDP Protocol – Packet Header Specification

This document defines the binary packet header format used by all UDP messages exchanged between R-Type clients and servers.  
The header is fully deterministic, endian-safe, and designed to support lightweight reliability on top of UDP (similar to Quake III Arena, ENet, and other FPS protocols).

---

## 1. Goals for the Header

The header must provide:

1. **Protocol versioning**  
   Allows clients/servers to reject incompatible versions.

2. **Message classification**  
   Identifies the type of message (Input, Snapshot, Ping, Spawn, etc.).

3. **Reliability metadata**  
   Sequence numbers + ACK + ACK bitmask.
   Enables the engine to:
   - track received packets,
   - know which packets the other side has received,
   - manage retransmission of reliable messages.

4. **Timestamp**  
   Used for latency estimation and authoritative timekeeping.

5. **Compact binary layout**  
   Designed to reduce bandwidth usage.

---

## 2. Binary Layout (On-the-wire format)

All fields use **little-endian** encoding.  
All integers are fixed-size, no padding, no alignment assumptions.

| Field Name     | Type              | Size | Description |
|----------------|-------------------|------|-------------|
| `version`      | `uint16`          | 2 B  | Protocol version used by both ends |
| `msgType`      | `uint8`           | 1 B  | Enum identifying the message payload type |
| `flags`        | `uint8`           | 1 B  | Bitfield: reliable, compressed, connectionless… |
| `sequence`     | `uint32`          | 4 B  | Increasing sequence number for outgoing packets |
| `ack`          | `uint32`          | 4 B  | Last sequence received from the peer |
| `ackBits`      | `uint32`          | 4 B  | 32-bit window tracking `ack - 1` … `ack - 32` |
| `timestampMs`  | `uint32`          | 4 B  | Sender timestamp in ms |

**Total size: 2 + 1 + 1 + 4 + 4 + 4 + 4 = 20 bytes**

Every R-Type packet begins with these 20 bytes.

---

## 3. C++ Structure (Exact Struct)

```cpp
#pragma pack(push, 1)
struct Header {
    std::uint16_t version;      // Protocol version
    std::uint8_t  msgType;      // MessageType enum
    std::uint8_t  flags;        // Bitfield (reliable, compressed, etc.)
    std::uint32_t sequence;     // Sender packet sequence
    std::uint32_t ack;          // Last received sequence from peer
    std::uint32_t ackBits;      // Bitmap for the previous 32 packets
    std::uint32_t timestampMs;  // Timestamp for ping/latency
};
#pragma pack(pop)
