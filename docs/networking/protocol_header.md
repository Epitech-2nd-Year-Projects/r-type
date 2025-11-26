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

| Field Name     | Type      | Size | Description |
|----------------|-----------|------|-------------|
| `version`      | `uint16`  | 2 B  | Protocol version used by both ends |
| `msgType`      | `uint8`   | 1 B  | Enum identifying the message payload type |
| `flags`        | `uint8`   | 1 B  | Bitfield: reliable, compressed, connectionless… |
| `sequence`     | `uint32`  | 4 B  | Increasing sequence number for outgoing packets |
| `ack`          | `uint32`  | 4 B  | Last sequence received from the peer |
| `ackBits`      | `uint32`  | 4 B  | 32-bit window tracking `ack - 1` … `ack - 32` |
| `timestampMs`  | `uint32`  | 4 B  | Sender timestamp in ms |

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
```

Notes:

- `#pragma pack` ensures no padding, but your serialization will **explicitly write fields** in order, so padding is irrelevant.
- The struct is **not** directly memcpy’d to the network.
- Encoding must be done using `engine::net::BufferWriter`.

---

## 4. Field Semantics (Very Detailed)

### 4.1. `version` (uint16)

Allows version negotiation:

- If versions mismatch → the server sends a `JoinReject` explaining the error.

### 4.2. `msgType` (uint8)

Example enumeration:

```text
0 = Invalid
1 = Hello
2 = JoinRequest
3 = JoinAccept
4 = JoinReject
5 = InputState
6 = WorldSnapshot
7 = SpawnEntity
8 = DestroyEntity
9 = PlayerDied
10 = ClientCommand
11 = ServerCommand
12 = Ping
13 = Pong
```

### 4.3. `flags` (uint8)

Bitfield proposal:

| Bit | Mask   | Meaning                       |
|-----|--------|-------------------------------|
| 0   | 0x01   | Reliable message present      |
| 1   | 0x02   | Compressed payload            |
| 2   | 0x04   | Connectionless (no session)   |
| 3–7 | –      | Reserved for future use       |

### 4.4. `sequence` (uint32)

Incremented for every outgoing packet.

Example:

- Client sends: 1, 2, 3, 4, 5 …  
- Server independently sends: 1, 2, 3, 4 …

Used to:

- detect ordering,  
- track which packets carried reliable messages,  
- compute packet loss.

### 4.5. `ack` (uint32)

Acknowledges the most recently received packet sequence.

If client received packets:

```text
Server seq received: 10, 11, 13
```

Client will send:

```text
ack = 13
```

### 4.6. `ackBits` (uint32)

Bitmask representing:

```text
ack - 1
ack - 2
ack - 3
...
ack - 32
```

Example:

- Server sent packets: `9, 10, 11, 12`  
- Client received only: `10, 12`  
- If `ack = 12`, `ackBits` will encode:

```text
bit 0 → sequence 11 → 0
bit 1 → sequence 10 → 1
bit 2 → sequence 9  → 0
...
```

Used to:

- detect loss without requiring retransmissions of snapshots/inputs,  
- know exactly which reliable messages to resend.

### 4.7. `timestampMs` (uint32)

Server or client local time in ms.

Used for:

- latency computation (`RTT = now - timestampMs`),  
- lag compensation,  
- ordering corrections.

---

## 5. Example (Hex Dump)

Header fields:

| Field        | Value      |
|--------------|------------|
| version      | 0x0001     |
| msgType      | 0x03 (JoinAccept for example) |
| flags        | 0x01 (Reliable) |
| sequence     | 0x00000042 |
| ack          | 0x0000003F |
| ackBits      | 0x00000019 |
| timestampMs  | 0x00012345 |

Binary (little endian):

```text
01 00        // version
03           // msgType
01           // flags
42 00 00 00  // sequence
3F 00 00 00  // ack
19 00 00 00  // ackBits
45 23 01 00  // timestampMs
```

---

## 6. Engine Serialization Rules

The struct MUST be serialized using:

- `engine::net::BufferWriter::write<std::uint16_t>()`  
- `write<std::uint8_t>()`  
- `write<std::uint32_t>()`  

No direct `memcpy`, no `reinterpret_cast` of the struct.

This guarantees:

- deterministic endianness,  
- no undefined behavior,  
- compatibility with different platforms/architectures.

---

## 7. Validation Rules

Upon decoding a header:

- Reject packets with total size `< 20` bytes.  
- Reject unknown `msgType` values.  
- Reject unsupported `version` values.  
- Optionally log malformed sequences or suspicious patterns.  
- Keep the channel alive unless the packet is clearly invalid / malicious.

---

## 8. Summary

This header provides:

- Versioning  
- Message categorization  
- Reliability (sequence + ack + ackBits)  
- Timestamp  
- Small footprint (20 bytes)

It enables:

- Quake 3–like reliability behavior,  
- Delta snapshots and rolling windows,  
- Input redundancy and smooth state synchronization.
