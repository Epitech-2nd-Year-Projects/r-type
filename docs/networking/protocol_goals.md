# R-Type UDP Protocol – Goals and Constraints

## 1. Scope

This document defines the goals, constraints, and high-level design choices for the R-Type UDP protocol library.

The protocol sits between:
- The **engine networking layer** (`engine::net::Client`, `engine::net::Server`, buffers, serialization).
- The **game logic** (R-Type rules, entities, ECS, inputs).

The protocol is responsible for:
- Defining binary messages exchanged between client and server.
- Implementing a Quake3-style UDP model with:
  - Authoritative server.
  - Client-side prediction.
  - Snapshot-based world replication.
  - Lightweight reliability layer over UDP.

Out of scope:
- Game rules and ECS implementation.
- Low-level socket management (handled by `engine::net`).
- Transport other than UDP for in-game traffic.

---

## 2. High-level Architecture

### 2.1. Network model

- **Authoritative server**:
  - The server is the single source of truth for game state.
  - Clients never decide the “real” world state, they only predict locally.

- **UDP-only for in-game traffic**:
  - All moment-to-moment gameplay data (inputs, snapshots) uses UDP.
  - No TCP dependency in the main gameplay loop.

- **Client-side prediction & interpolation**:
  - Clients send input commands frequently.
  - Clients predict their own movement locally.
  - Server sends snapshots of the world at a fixed tickrate.
  - Clients interpolate between snapshots for smooth rendering.

- **Quake3-inspired design**:
  - Sequence numbers and ACKs in every packet.
  - Some messages are reliable (retransmitted until ACKed).
  - Inputs and snapshots are mostly unreliable but redundant.

### 2.2. Position in the engine

- The protocol depends on:
  - `engine::net` for:
    - UDP send/receive abstraction.
    - Buffer read/write utilities.
  - Common engine types (fixed-size ints, time utilities).

- The protocol does **not**:
  - Own or manage OS sockets directly.
  - Know about ECS, components, or rendering.
  - Depend on any R-Type specific game code.

The layering is:

`[Game logic] <-> [Protocol lib] <-> [Engine::net] <-> [OS / UDP sockets]`

---

## 3. Traffic Classes and Reliability

We split all network traffic into **two classes**:

### 3.1. Reliable messages

Characteristics:
- Must be delivered **exactly once**, or the connection is considered broken.
- Rare but critical for correctness and UX.

Examples:
- Connection / join / leave:
  - `Hello`, `JoinRequest`, `JoinAccept`, `JoinReject`.
- Game lifecycle:
  - `GameStart`, `GameOver`, level transitions.
- Out-of-band information:
  - Chat messages, error notifications.

Delivery mechanism:
- Implemented using:
  - Per-channel sequence numbers.
  - ACK and ACK bitmask in the header.
- Reliable messages are re-sent with new packets until:
  - The sender sees them acknowledged by the receiver.
- The receiver deduplicates reliable messages using:
  - Reliable IDs or sequence tracking.

### 3.2. Unreliable messages

Characteristics:
- Losing one message is acceptable.
- Always superseded by more recent data.
- Never individually retransmitted.

Examples:
- Client → Server:
  - `InputState` (player movement, shooting).
- Server → Client:
  - `WorldSnapshot` (positions, states of entities).
  - `Ping` / `Pong` (for latency estimate can also be unreliable).

Mechanisms:
- Inputs are sent **frequently** and often duplicated (rolling window).
- Snapshots are **never re-sent**; only new snapshots are sent.
- If a snapshot is lost, the next one is based on the last known acknowledged snapshot.

---

## 4. World Synchronization Model

### 4.1. Server tick and snapshots

- The server runs at a fixed tickrate (e.g. 20–60 Hz).
- At each tick:
  - Game logic is updated.
  - A **world snapshot** is produced:
    - List of network-relevant entities (`EntityNetState`).
    - Snapshot ID and server tick.

- The server stores the last N snapshots in a history buffer:
  - Allows sending deltas based on previous snapshots.

### 4.2. Snapshot delta compression

- Each `WorldSnapshot` can be sent:
  - As a **full snapshot** (no base).
  - As a **delta** from a previous snapshot known by the client.

- Delta rules:
  - New entities → fully described in the delta.
  - Removed entities → marked with a “deleted” flag/opcode.
  - Existing entities → only changed fields are encoded.

- Benefits:
  - Bandwidth reduction.
  - Scales well with many entities even if only a few move.

### 4.3. Client interpolation & extrapolation

- The client keeps a buffer of reconstructed full snapshots.
- Rendering is done “in the past”:
  - The client interpolates between two snapshots around the render timestamp.
- If a snapshot is missing:
  - The client extrapolates for a short time using velocities.
  - Beyond a maximum extrapolation time, entities can be frozen or hidden.

---

## 5. Input and Prediction Model

### 5.1. Client → Server input commands

- Clients do not send positions; they send **inputs**:
  - Direction buttons (up/down/left/right).
  - Fire button.
  - Optional analog axes.

- Each input command includes:
  - A local input sequence number.
  - A timestamp.
  - A bitfield of pressed actions.

- Inputs are batched:
  - Each outgoing packet contains the latest input + a small window of previous inputs (redundancy).

### 5.2. Client-side prediction

- The client applies its own inputs immediately to a local copy of the player state.
- This gives instant feedback even with non-zero latency.
- When a new server snapshot is received:
  - The client reconciles the authoritative state with its predicted one.
  - Small deviations are smoothed out by interpolation.
  - Large errors may trigger a hard correction (snap).

---

## 6. Constraints and Non-Functional Requirements

### 6.1. Performance constraints

- Bandwidth budget:
  - Designed to work over typical home internet with moderate upload/download.
  - Prefer small packets; avoid huge MTU-sized messages.
- CPU budget:
  - Serialization/deserialization must be low overhead.
  - Delta encoding/decoding should be straightforward bit/field operations.
- No blocking operations in the main game loop:
  - All networking runs through `engine::net` asynchronous primitives.

### 6.2. Robustness and security

- Never trust the client:
  - All gameplay logic runs on the server.
  - Protocol validates packet sizes, ranges, and IDs.
- Malformed packets:
  - Must never crash the server or the client.
  - Are logged, counted, and discarded.
- Versioning:
  - Every packet carries a protocol version.
  - Mismatched versions trigger a clean disconnect or a clear error.

### 6.3. Portability

- Only uses:
  - Fixed-size integer types (`std::uint8_t`, `std::uint16_t`, etc.).
  - Explicit endianness through engine serialization helpers.
- No assumptions about struct packing on disk.
- The same wire format must be readable on any supported platform.

---

## 7. Dependencies and Integration

- The protocol library depends on:
  - `engine::net` abstractions (buffers, endpoints, client/server).
  - Common engine utilities (time, logging).

- The protocol library does **not** depend on:
  - Game-specific ECS types.
  - Rendering or audio subsystems.
  - Platform-specific socket APIs.

Integration points:

- **Server**:
  - On incoming datagram:
    - `protocol::decode` → produce `Packet` → translate to game events.
  - On tick:
    - Build `WorldSnapshot` → `protocol::encode` → send via `engine::net::Server`.

- **Client**:
  - On local input:
    - Build `InputState` → enqueue into protocol channel.
  - On incoming datagram:
    - `protocol::decode` → update snapshot buffer / reliable messages.
  - For rendering:
    - Query interpolated snapshot from protocol’s client-side buffer.

---

## 8. Summary

The R-Type UDP protocol is:

- A **Quake3-inspired**, snapshot-based UDP protocol.
- Built around:
  - Authoritative server.
  - Client-side prediction.
  - Delta-compressed snapshots.
  - Lightweight reliability on top of UDP.
- Implemented as a **standalone library** that:
  - Depends only on engine networking primitives.
  - Exposes a clean API to the server and client code.
