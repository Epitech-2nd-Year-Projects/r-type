                                                   R-TYPE
                                      REAL-TIME UDP PROTOCOL
                                                  v1.0

Author: Laurent ALIU 
Last Updated: 2025-12-16

1. Introduction
---------------
This document specifies the UDP-based network protocol for the R-Type game server and clients. It defines on-wire packet formats, message catalogue, reliability semantics, and communication flows. The goal is to enable interoperable implementations in any language (e.g., Python, Java) without relying on C++ types.

2. Document Conventions
-----------------------
- Transport: UDP datagrams.
- Endianness: Little-endian for all multi-byte numeric fields.
- Primitive types: `u8` (8-bit unsigned), `u16` (16-bit unsigned), `u32` (32-bit unsigned), `i16` (16-bit signed), `f32` (32-bit float, IEEE754).
- Text: ASCII/UTF-8, length-prefixed when variable-length. Unless noted, string lengths are encoded as `u8` followed by raw bytes.
- Diagrams: Offsets are byte offsets from start of the packet header or payload.

3. Protocol Overview
--------------------
All packets share a fixed 20-byte header followed by a message-specific payload. The header carries sequencing, acknowledgement, flags (reliable/connectionless/compressed), and a timestamp. Reliability is implemented with sequence numbers plus `Ack` and `AckBits`; reliable messages are re-sent until acknowledged. Some messages are marked connectionless (usable before a session is established). Compression is reserved and not currently in use.

4. Packet Header (20 bytes)
---------------------------
```
0                   7 8                  15 16                 23 24                 31
+0  u16 Version     +4  u32 Sequence     +12 u32 AckBits       +16 u32 TimestampMs
+2  u8  MsgType     +8  u32 Ack
+3  u8  Flags
```

Field descriptions
- Version (`u16`): Protocol version. Current value: `1`.
- MsgType (`u8`): Message ID (see Section 7).
- Flags (`u8` bitfield):
  - bit0 `reliable`: Packet includes at least one reliable message; sender expects acknowledgments.
  - bit1 `compressed`: Payload compressed (reserved, unused).
  - bit2 `connectionless`: Packet can be processed without a joined game session.
  - bits3..7: Reserved.
- Sequence (`u32`): Sender’s packet sequence number (wraps modulo 2^32).
- Ack (`u32`): Highest sequence number received from the peer.
- AckBits (`u32`): Bitmap for the 32 packets before `Ack`; bit0 acknowledges `Ack-1`, bit31 acknowledges `Ack-32`.
- TimestampMs (`u32`): Sender timestamp in milliseconds, used for latency estimation/diagnostics.

5. Reliability Model
--------------------
- Sequencing: “More recent” comparison uses half-range arithmetic (2^31) to handle wrap-around.
- Acknowledgment window: `AckBits` provides selective acks for the 32 packets immediately prior to `Ack`.
- Reliable delivery: Messages marked reliable are queued for re-send until acknowledged. Default resend timeout: 250 ms. Maximum pending reliable packets tracked per peer: 64.
- Connectionless: Messages flagged connectionless may be exchanged without a joined session; sequence/ack fields are still present.
- Loss handling: Unreliable messages (e.g., snapshots, inputs) may be dropped; higher layers tolerate loss via redundancy (inputs) or deltas (snapshots).

6. Constants and Enumerations
----------------------------
- Header flags: bit0 `reliable`, bit1 `compressed` (reserved), bit2 `connectionless`, bits3..7 reserved.
- JoinReject reasons: 0 Unknown, 1 VersionMismatch, 2 ServerFull, 3 InvalidRoom, 4 Banned.
- PlayerDied causes: 0 Unknown, 1 Enemy, 2 Projectile, 3 Obstacle, 4 Suicide, 5 Void.
- WorldSnapshot ops: 0 Create, 1 Update, 2 Delete.
- WorldSnapshot full/no-base sentinel: `NO_BASE_SNAPSHOT = 0xFFFFFFFF`.
- Input buttons bitfield: bit0 Up, bit1 Down, bit2 Left, bit3 Right, bit4 Fire, bit5 AltFire.
- Command IDs (well-known): 1 StartGame, 2 SetReady, 3 Unready, 4 ChatMessage, 5 DisconnectNotice.

7. Message Catalogue
--------------------
Legend: Dir = direction, Rel = reliable, CL = connectionless. Dir values: C→S (client to server), S→C (server to client), Both (either).

| ID (hex) | Name               | Dir   | Rel | CL  | Purpose |
|----------|--------------------|-------|-----|-----|---------|
| 0x00     | Invalid            | –     | no  | yes | Reserved |
| 0x01     | Hello              | C→S   | no  | yes | Optional probe (ignored by server) |
| 0x02     | JoinRequest        | C→S   | yes | yes | Request to join a room |
| 0x03     | JoinAccept         | S→C   | yes | yes | Join success and session params |
| 0x04     | JoinReject         | S→C   | yes | yes | Join refusal with reason/message |
| 0x05     | InputState         | C→S   | no  | no  | Player input (redundant window) |
| 0x06     | WorldSnapshot      | S→C   | no  | no  | World state full/delta snapshot |
| 0x07     | SpawnEntity        | S→C   | yes | no  | Reserved for explicit spawn |
| 0x08     | DestroyEntity      | S→C   | yes | no  | Reserved for explicit destroy |
| 0x09     | PlayerDied         | S→C   | yes | no  | Player death notification |
| 0x0A     | ClientCommand      | C→S   | yes | no  | Generic client command |
| 0x0B     | ServerCommand      | S→C   | yes | no  | Generic server command/notice |
| 0x0C     | Ping               | C→S   | no  | yes | Latency probe |
| 0x0D     | Pong               | S→C   | no  | yes | Latency response |
| 0x0E     | RoomListRequest    | C→S   | yes | yes | Request public room directory |
| 0x0F     | RoomListResponse   | S→C   | yes | yes | Room directory snapshot |
| 0x10     | CreateRoomRequest  | C→S   | yes | yes | Request to create a room |
| 0x11     | CreateRoomResponse | S→C   | yes | yes | Room creation result |

8. Payload Specifications
-------------------------
All strings are raw ASCII/UTF-8 bytes. “len” fields are lengths, not null-terminated.

8.1 Hello (0x01)
- No payload (reserved/ignored).

8.2 JoinRequest (0x02)
- `u16 client_version`
- `u8 name_len` (0..31), `name`
- `u8 room_len` (0..15), `room_code`
- `u8 pass_len` (0..15), `room_password` (optional)

8.3 JoinAccept (0x03)
- `u16 server_version`
- `u32 player_id`
- `u8 max_players`
- `u8 tick_rate` (Hz)
- `u32 seed` (random seed for deterministic systems)

8.4 JoinReject (0x04)
- `u16 server_version`
- `u8 reason` (0 Unknown, 1 VersionMismatch, 2 ServerFull, 3 InvalidRoom, 4 Banned)
- `u8 msg_len` (0..63), `message`

8.5 InputState (0x05)
- `u8 command_count` (1..4 recent commands)
- For each command:
  - `u32 input_sequence`
  - `u8 buttons` (bit0 Up, bit1 Down, bit2 Left, bit3 Right, bit4 Fire, bit5 AltFire)
  - `i16 analog_x`
  - `i16 analog_y`
  - `u32 client_time_ms`

8.6 WorldSnapshot (0x06)
- `u32 snapshot_id`
- `u32 base_snapshot_id` (`NO_BASE_SNAPSHOT = 0xFFFFFFFF` means full snapshot)
- `u32 server_tick`
- `u16 delta_count`
- For each delta:
  - `u8 op` (0 Create, 1 Update, 2 Delete)
  - `u32 entity_id`
  - If Create: `u16 type`, `i16 x`, `i16 y`, `i16 vx`, `i16 vy`, `u8 hp`, `u8 flags`
  - If Delete: no further fields
  - If Update:
    - `u8 field_mask` (bit0 type, bit1 x, bit2 y, bit3 vx, bit4 vy, bit5 hp, bit6 flags)
    - Only the fields whose bits are set are present, in the same order as Create

8.7 SpawnEntity (0x07)
- Reserved (no current payload defined).

8.8 DestroyEntity (0x08)
- Reserved (no current payload defined).

8.9 PlayerDied (0x09)
- `u32 player_id`
- `u32 killer_entity_id` (0 if none)
- `u8 cause` (0 Unknown, 1 Enemy, 2 Projectile, 3 Obstacle, 4 Suicide, 5 Void)
- `u8 remaining_lives`

8.10 ClientCommand / ServerCommand (0x0A / 0x0B)
- `u16 command_id`
- `u16 payload_len`
- `payload` (0..65535 bytes)
- Well-known `command_id` values:
  - 1 StartGame
  - 2 SetReady
  - 3 Unready
  - 4 ChatMessage
  - 5 DisconnectNotice

8.11 Ping (0x0C)
- `u32 client_time_ms`

8.12 Pong (0x0D)
- `u32 client_time_ms` (echoed)
- `u32 server_time_ms` (timestamp when Pong was sent)

8.13 RoomListRequest (0x0E)
- Empty payload.

8.14 RoomListResponse (0x0F)
- `u8 count` (0..64)
- For each room:
  - `u8 code_len` (<=15), `room_code`
  - `u8 name_len` (<=31), `room_name`
  - `u8 is_private` (0/1)
  - `u8 player_count`
  - `u8 max_players`

8.15 CreateRoomRequest (0x10)
- `u8 name_len` (<=31), `room_name`
- `u8 is_private` (0/1)
- `u8 max_players` (1..255; server clamps to config)
- `u8 pass_len` (<=15), `room_password` (required for private when present)

8.16 CreateRoomResponse (0x11)
- `u8 success` (0/1)
- `u8 msg_len` (0..63), `message`
- `u8 has_room` (0/1)
- If has_room: one RoomSummary (same layout as a RoomList entry)
- `u8 pass_len` (<=15), `room_password` (always present; populated for private rooms)

9. Communication Flows
----------------------
9.1 Handshake (Join)
1) Client starts UDP transport.
2) Client sends JoinRequest (reliable, connectionless) every 500 ms until a response, up to 5 attempts.
3) Server validates version, room existence/password, capacity.
4) On success: JoinAccept returns `player_id`, `tick_rate`, `seed`, `max_players`; client enters joined state.
5) On failure: JoinReject with reason/message; client may retry or abort.
6) Hello (0x01) is ignored; no session state is created.

9.2 Lobby Operations
- RoomListRequest/Response and CreateRoomRequest/Response are connectionless and reliable; usable before joining a game room.

9.3 Gameplay Loop
- Inputs: Client sends InputState at a configurable 30–120 Hz (default ~60 Hz). Each payload carries up to the 4 most recent commands for redundancy.
- Snapshots: Server runs a fixed tick loop (default 60 Hz) and emits one WorldSnapshot per tick to each joined player. Snapshots may be full or delta (base_snapshot_id ≠ 0xFFFFFFFF).
- Events: PlayerDied and ServerCommand are reliable and re-sent until acknowledged.
- Commands: ClientCommand is reliable; used for ready/unready, chat, disconnect notice, etc.

9.4 Liveness and Time
- Ping/Pong: Client sends Ping roughly every 1 s; server replies with Pong. Client estimates RTT and clock offset as:
  - RTT ≈ `now_ms - client_time_ms`
  - offset ≈ `server_time_ms - (client_time_ms + RTT/2)`
- Timeout: Server disconnects peers after ~15 s of inactivity. A DisconnectNotice (ServerCommand) may be sent if possible.

10. Loss, Ordering, and Resends
------------------------------
- Reliable messages: Retained and re-sent on 250 ms timeout until Ack/AckBits confirm receipt.
- Unreliable messages: May be dropped. InputState mitigates loss via redundancy (last N commands). WorldSnapshot mitigates via full snapshots or by requesting latest state (application-level).
- Ordering: Sequence numbers are per-peer. More-recent test uses half-range (2^31) to handle wrap.
- Ack interpretation: A packet with sequence X is acknowledged if X == Ack, or if X falls within the 32-packet window behind Ack and the corresponding bit in AckBits is set.

11. Timing Parameters (defaults)
--------------------------------
- Server tick rate: 60 Hz (configurable).
- Client input send rate: 60 Hz default; clamped 30–120 Hz.
- Ping interval: ~1000 ms.
- Reliable resend timeout: 250 ms.
- Reliable queue capacity: 64 pending packets.
- Peer inactivity timeout: ~15 s.
- Room idle reclamation (empty rooms): ~30 s.

12. Notes for Implementers
--------------------------
- Always populate Sequence, Ack, and AckBits, even for connectionless packets; this enables RTT and loss tracking.
- When sending reliable packets, set the reliable flag in the header; resend until acknowledged.
- For InputState, include multiple recent commands to survive a single packet loss.
- For WorldSnapshot, apply Update deltas relative to the referenced base snapshot; if base is missing, wait for or request a full snapshot (base_snapshot_id = 0xFFFFFFFF).
- Length fields bound strings: truncate inputs to specified maxima before encoding.
