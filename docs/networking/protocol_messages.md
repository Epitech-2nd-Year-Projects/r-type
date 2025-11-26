# R-Type UDP Protocol – Message Types and Payloads

This document enumerates all R-Type UDP protocol message types and defines their binary payload structures.  
Every message is sent inside a UDP datagram, prefixed by the common `Header` described in `protocol_header.md`.

All integer fields use **little-endian** encoding and fixed-size types (`std::uint8_t`, `std::uint16_t`, `std::uint32_t`, etc.).  
Strings are length-prefixed and bounded to avoid untrusted variable-length data.

---

## 1. MessageType Enumeration

The `msgType` field in the `Header` is a `uint8` that takes one of the following values:

```cpp
enum class MessageType : std::uint8_t {
    Invalid        = 0,  // Reserved / not used

    // Connection / session
    Hello          = 1,  // Optional connectionless hello / ping server
    JoinRequest    = 2,  // Client asks to join a game
    JoinAccept     = 3,  // Server accepts and assigns playerId
    JoinReject     = 4,  // Server rejects with a reason code

    // Gameplay
    InputState     = 5,  // Client → Server: player input commands
    WorldSnapshot  = 6,  // Server → Client: world state snapshot (full or delta)
    SpawnEntity    = 7,  // Server → Client: explicit spawn (optional if not in snapshots)
    DestroyEntity  = 8,  // Server → Client: explicit destroy (optional if not in snapshots)
    PlayerDied     = 9,  // Server → Client: notification that a player died

    // Generic commands / events
    ClientCommand  = 10, // Client → Server: generic reliable commands (chat, ready, etc.)
    ServerCommand  = 11, // Server → Client: generic reliable commands (text, notifications)

    // Utility
    Ping           = 12, // Client → Server: ping with timestamp
    Pong           = 13  // Server → Client: pong echoing data
};
```

The `MessageType` values are stable and form part of the protocol contract.

---

## 2. Connection and Session Messages

### 2.1. Hello (MessageType::Hello)

**Direction:** Client → Server  
**Reliability:** Can be connectionless or reliable, depending on usage.  
**Purpose:** Optional discovery / server info request (can be skipped if not needed).

**Payload format:**

| Field           | Type     | Description                        |
|-----------------|----------|------------------------------------|
| `clientVersion` | `uint16` | Client protocol version            |
| `flags`         | `uint16` | Reserved for future use           |

```cpp
struct HelloPayload {
    std::uint16_t clientVersion;
    std::uint16_t flags; // currently unused (0)
};
```

---

### 2.2. JoinRequest (MessageType::JoinRequest)

**Direction:** Client → Server  
**Reliability:** Reliable  
**Purpose:** Request to join a game instance, providing basic identity and room.

**String encoding:**  

- Strings are encoded as: `uint8 length` followed by `length` bytes (UTF-8).  
- Maximum length is bounded (e.g. 31 characters).

**Payload format:**

| Field           | Type     | Description                             |
|-----------------|----------|-----------------------------------------|
| `clientVersion` | `uint16` | Client protocol version                 |
| `playerNameLen` | `uint8`  | Length of `playerName` in bytes         |
| `playerName`    | `uint8[]`| UTF-8 player name (0–31 bytes)         |
| `roomCodeLen`   | `uint8`  | Length of `roomCode` in bytes           |
| `roomCode`      | `uint8[]`| UTF-8 room identifier (0–15 bytes)     |

```cpp
struct JoinRequestPayload {
    std::uint16_t clientVersion;
    std::uint8_t  playerNameLen; // 0..31
    // followed by playerNameLen bytes
    // followed by:
    std::uint8_t  roomCodeLen;   // 0..15
    // followed by roomCodeLen bytes
};
```

---

### 2.3. JoinAccept (MessageType::JoinAccept)

**Direction:** Server → Client  
**Reliability:** Reliable  
**Purpose:** Confirm that the client joined successfully and assign identifiers.

**Payload format:**

| Field          | Type     | Description                              |
|----------------|----------|------------------------------------------|
| `serverVersion`| `uint16` | Server protocol version                  |
| `playerId`     | `uint32` | Unique player ID assigned by the server  |
| `maxPlayers`   | `uint8`  | Maximum players in this game instance    |
| `tickRate`     | `uint8`  | Server tickrate (simulation steps/sec)   |
| `seed`         | `uint32` | Random seed for deterministic systems    |

```cpp
struct JoinAcceptPayload {
    std::uint16_t serverVersion;
    std::uint32_t playerId;
    std::uint8_t  maxPlayers;
    std::uint8_t  tickRate;
    std::uint32_t seed;
};
```

---

### 2.4. JoinReject (MessageType::JoinReject)

**Direction:** Server → Client  
**Reliability:** Reliable  
**Purpose:** Inform the client that join failed and why.

**Payload format:**

| Field          | Type     | Description                                    |
|----------------|----------|------------------------------------------------|
| `serverVersion`| `uint16` | Server protocol version                        |
| `reasonCode`   | `uint8`  | Enum indicating the cause of rejection         |
| `msgLen`       | `uint8`  | Length of human-readable message in bytes      |
| `msg`          | `uint8[]`| UTF-8 message (0–63 bytes)                     |

```cpp
enum class JoinRejectReason : std::uint8_t {
    Unknown          = 0,
    VersionMismatch  = 1,
    ServerFull       = 2,
    InvalidRoom      = 3,
    Banned           = 4
};

struct JoinRejectPayload {
    std::uint16_t       serverVersion;
    std::uint8_t        reasonCode; // JoinRejectReason
    std::uint8_t        msgLen;     // 0..63
    // followed by msgLen bytes (UTF-8)
};
```

---

## 3. Gameplay Messages

### 3.1. InputState (MessageType::InputState)

**Direction:** Client → Server  
**Reliability:** Unreliable but redundant (sent frequently, rolling window)  
**Purpose:** Transmit player inputs instead of positions.

We use a bitfield for R-Type actions (2D shoot’em up):

```cpp
enum InputButtons : std::uint16_t {
    Input_Up       = 1 << 0,
    Input_Down     = 1 << 1,
    Input_Left     = 1 << 2,
    Input_Right    = 1 << 3,
    Input_Shoot    = 1 << 4,
    Input_Bomb     = 1 << 5,
    Input_Slow     = 1 << 6
};
```

**Payload format:**

| Field           | Type     | Description                                  |
|-----------------|----------|----------------------------------------------|
| `inputSeq`      | `uint32` | Monotonically increasing input sequence ID   |
| `buttons`       | `uint16` | Bitfield of pressed buttons                  |
| `analogX`       | `int16`  | Optional analog X (0 if unused)             |
| `analogY`       | `int16`  | Optional analog Y (0 if unused)             |
| `clientTimeMs`  | `uint32` | Client local time when input was sampled     |

```cpp
struct InputStatePayload {
    std::uint32_t inputSeq;
    std::uint16_t buttons;      // InputButtons bitfield
    std::int16_t  analogX;      // 0 if not used
    std::int16_t  analogY;      // 0 if not used
    std::uint32_t clientTimeMs; // for prediction / reconciliation
};
```

---

### 3.2. WorldSnapshot (MessageType::WorldSnapshot)

**Direction:** Server → Client  
**Reliability:** Unreliable (never retransmitted)  
**Purpose:** Send the authoritative world state at a given server tick.

The actual bytes on the wire use a **delta** encoding and are defined in the snapshot/delta document, but this section defines the logical content.

**Entity representation (logical):**

```cpp
struct EntityNetState {
    std::uint32_t entityId;     // Stable network ID
    std::uint16_t type;         // Archetype / type code (player, enemy, missile, etc.)
    std::int16_t  x;            // Quantized position X
    std::int16_t  y;            // Quantized position Y
    std::int16_t  vx;           // Quantized velocity X
    std::int16_t  vy;           // Quantized velocity Y
    std::uint8_t  hp;           // Hit points
    std::uint8_t  flags;        // Alive, invincible, etc.
};
```

**Payload format (logical):**

| Field           | Type     | Description                                      |
|-----------------|----------|--------------------------------------------------|
| `snapshotId`    | `uint32` | Unique ID of this snapshot                       |
| `baseSnapshotId`| `uint32` | ID of base snapshot for delta (or 0xFFFFFFFF)   |
| `serverTick`    | `uint32` | Server simulation tick                           |
| `entityCount`   | `uint16` | Number of entities encoded in this snapshot      |
| `entities[]`    | variable | Delta-encoded entity states (format see delta doc)|

```cpp
struct WorldSnapshotPayloadHeader {
    std::uint32_t snapshotId;
    std::uint32_t baseSnapshotId; // 0xFFFFFFFF if full snapshot
    std::uint32_t serverTick;
    std::uint16_t entityCount;
    // followed by delta-encoded entities
};
```

The exact binary encoding of `entities[]` (full vs delta) is specified in the snapshot/delta specification.

---

### 3.3. SpawnEntity (MessageType::SpawnEntity) – optional

**Direction:** Server → Client  
**Reliability:** Reliable (if used)  
**Purpose:** Explicitly spawn an entity, if not handled purely via snapshots.

This message can be omitted if the game relies solely on snapshots for entity creation. It is useful for “instant” events or for debugging.

**Payload format:**

| Field        | Type     | Description                         |
|--------------|----------|-------------------------------------|
| `entityId`   | `uint32` | Network ID of the new entity        |
| `type`       | `uint16` | Archetype/type code                 |
| `x`          | `int16`  | Initial X position (quantized)      |
| `y`          | `int16`  | Initial Y position (quantized)      |
| `hp`         | `uint8`  | Initial hit points                  |
| `flags`      | `uint8`  | Initial flags                       |

```cpp
struct SpawnEntityPayload {
    std::uint32_t entityId;
    std::uint16_t type;
    std::int16_t  x;
    std::int16_t  y;
    std::uint8_t  hp;
    std::uint8_t  flags;
};
```

---

### 3.4. DestroyEntity (MessageType::DestroyEntity)

**Direction:** Server → Client  
**Reliability:** Reliable (if used)  
**Purpose:** Explicitly notify that an entity should be removed.

**Payload format:**

| Field         | Type     | Description                             |
|---------------|----------|-----------------------------------------|
| `entityId`    | `uint32` | ID of the entity to destroy             |
| `reason`      | `uint8`  | Optional reason code                    |

```cpp
enum class DestroyReason : std::uint8_t {
    Unknown       = 0,
    Killed        = 1,
    Timeout       = 2,
    Despawned     = 3
};

struct DestroyEntityPayload {
    std::uint32_t entityId;
    std::uint8_t  reason; // DestroyReason
};
```

---

### 3.5. PlayerDied (MessageType::PlayerDied)

**Direction:** Server → Client  
**Reliability:** Reliable  
**Purpose:** Notify that a player died (for UI, sounds, score).

**Payload format:**

| Field          | Type     | Description                                  |
|----------------|----------|----------------------------------------------|
| `playerId`     | `uint32` | Player that died                             |
| `killerId`     | `uint32` | Entity that caused the death (or 0xFFFFFFFF) |
| `respawnTick`  | `uint32` | Tick at which the player can respawn        |

```cpp
struct PlayerDiedPayload {
    std::uint32_t playerId;
    std::uint32_t killerId;    // 0xFFFFFFFF if environment
    std::uint32_t respawnTick; // 0 if no respawn
};
```

---

## 4. Generic Command Messages

### 4.1. ClientCommand (MessageType::ClientCommand)

**Direction:** Client → Server  
**Reliability:** Reliable  
**Purpose:** Generic textual commands (chat, ready, emotes, etc.).

**Payload format:**

| Field        | Type     | Description                             |
|--------------|----------|-----------------------------------------|
| `cmdLen`     | `uint8`  | Length of UTF-8 command text            |
| `cmd`        | `uint8[]`| Command text (0–127 bytes)              |

```cpp
struct ClientCommandPayload {
    std::uint8_t cmdLen; // 0..127
    // followed by cmdLen bytes (UTF-8)
};
```

---

### 4.2. ServerCommand (MessageType::ServerCommand)

**Direction:** Server → Client  
**Reliability:** Reliable  
**Purpose:** Generic textual notifications, server messages, etc.

**Payload format:**

| Field        | Type     | Description                             |
|--------------|----------|-----------------------------------------|
| `cmdLen`     | `uint8`  | Length of UTF-8 command text            |
| `cmd`        | `uint8[]`| Command text (0–127 bytes)              |

```cpp
struct ServerCommandPayload {
    std::uint8_t cmdLen; // 0..127
    // followed by cmdLen bytes (UTF-8)
};
```

---

## 5. Utility Messages

### 5.1. Ping (MessageType::Ping)

**Direction:** Client → Server  
**Reliability:** Unreliable  
**Purpose:** Latency measurement, optional keep-alive.

**Payload format:**

| Field          | Type     | Description                           |
|----------------|----------|---------------------------------------|
| `pingId`       | `uint32` | Identifier to match Ping/Pong         |
| `clientTimeMs` | `uint32` | Client local time when ping was sent  |

```cpp
struct PingPayload {
    std::uint32_t pingId;
    std::uint32_t clientTimeMs;
};
```

---

### 5.2. Pong (MessageType::Pong)

**Direction:** Server → Client  
**Reliability:** Unreliable  
**Purpose:** Respond to ping.

**Payload format:**

| Field          | Type     | Description                               |
|----------------|----------|-------------------------------------------|
| `pingId`       | `uint32` | Echo of the `pingId` received             |
| `serverTimeMs` | `uint32` | Server local time when pong is generated  |

```cpp
struct PongPayload {
    std::uint32_t pingId;
    std::uint32_t serverTimeMs;
};
```

---

## 6. Summary

This document defines:

- A stable `MessageType` enum (1 byte in the header).  
- Structured payloads for:
  - Connection/session management.  
  - Input and world synchronization.  
  - Entity lifecycle events.  
  - Generic commands.  
  - Ping/pong.

All messages are:

- Compact.  
- Based on fixed-size integer types.  
- Designed to work with the reliability and snapshot model described in the other protocol documents.
