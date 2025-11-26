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
