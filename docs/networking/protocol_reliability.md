# R-Type UDP Protocol – Snapshot and Delta Model

This document defines how the R-Type server represents world snapshots and how it encodes **delta-compressed** updates for clients.
It builds on top of:
- The common packet header (`protocol_header.md`).
- The `WorldSnapshot` message definition (`protocol_messages.md`).
- The reliability policy (`protocol_reliability.md`).

The goal is to provide a **Quake3-style** snapshot system:
- Authoritative server snapshots at a fixed tickrate.
- Clients maintain a history of snapshots and interpolate between them.
- Snapshots are delta-compressed against a previous snapshot known by the client.
- Lost snapshots are tolerated; newer snapshots always supersede older ones.

---

## 1. High-Level Overview

### 1.1. Definitions

- **Snapshot**: A complete view of all network-relevant entities at a specific server tick.
- **Base snapshot**: A previous snapshot that the client is known to have received and applied.
- **Delta**: A compact representation of the difference between a current snapshot and a base snapshot.

### 1.2. Flow

1. The server simulates the game at a fixed tickrate (e.g., 30 or 60 Hz).
2. At some ticks (every tick or every N ticks), the server builds a **world snapshot** from ECS/game state.
3. The server stores snapshots in a **circular history buffer**.
4. For each connected client:
   - The server tracks which snapshot the client has acknowledged as the **last applied**.
   - When sending a new `WorldSnapshot`:
     - It chooses a base snapshot ID (usually the last acknowledged).
     - It encodes a delta between the current snapshot and this base.
5. The client:
   - Receives the delta, reconstructs the full snapshot using its local copy of the base.
   - Stores the reconstructed snapshot in a history buffer.
   - Uses interpolation/extrapolation for rendering.

---

## 2. Snapshot Data Model

### 2.1. EntityNetState

Each network-relevant entity is represented in snapshots by a compact structure:

```cpp
struct EntityNetState {
    std::uint32_t entityId;   // Stable unique ID over the entity's network lifetime
    std::uint16_t type;       // Type code (player, enemy, missile, etc.)
    std::int16_t  x;          // Quantized position X
    std::int16_t  y;          // Quantized position Y
    std::int16_t  vx;         // Quantized velocity X
    std::int16_t  vy;         // Quantized velocity Y
    std::uint8_t  hp;         // Hit points (0 = dead)
    std::uint8_t  flags;      // Status flags (alive, invincible, shield, etc.)
};
```

**Notes:**

- `entityId` is a stable ID assigned when the entity is created and reused in all snapshots until the entity is destroyed.
- `type` is a small, engine-defined code for the archetype (player ship, classic enemy, boss part, missile, power-up, etc.).
- Positions and velocities are **quantized** integers to reduce packet size and improve determinism:
  - Example: store positions in 1/16th of a pixel or in world units * 100.
- `hp` and `flags` cover most of the gameplay-visible state; further fields can be added as needed as long as the encoding is kept compact.

### 2.2. Full Snapshot Representation (Server-side)

On the server, a full snapshot can be represented as:

```cpp
struct Snapshot {
    std::uint32_t snapshotId;   // Monotonic ID
    std::uint32_t serverTick;   // Simulation tick
    std::vector<EntityNetState> entities;
};
```

- `snapshotId` is incremented for every generated snapshot (per game instance).
- `serverTick` is the tick count of the simulation when the snapshot was taken.
- `entities` contains the state for all relevant entities visible to that client (could be filtered by interest management in more complex games; for R-Type, it can be global).

---

## 3. Snapshot History (Server)

### 3.1. Circular Buffer

Each game instance maintains a **circular buffer** of the last `N` snapshots, e.g.:

```cpp
static constexpr std::size_t kMaxSnapshots = 64;

class SnapshotHistory {
public:
    void addSnapshot(const Snapshot& snapshot);
    const Snapshot* getSnapshot(std::uint32_t snapshotId) const;

private:
    Snapshot       _buffer[kMaxSnapshots];
    std::size_t    _head = 0; // index of the most recent snapshot
};
```

Behavior:

- `addSnapshot` stores a new snapshot at `_head`, increments `_head`, and overwrites the oldest snapshot when full.
- `getSnapshot(snapshotId)` searches the buffer for the requested ID and returns `nullptr` if not found (too old or not yet stored).

### 3.2. Per-Client State

For each client, the server tracks:

```cpp
struct ClientSnapshotState {
    std::uint32_t lastAckSnapshotId = 0; // last snapshot the client has confirmed
};
```

This information is used to choose the **base snapshot** when encoding a delta.

---

## 4. Delta Encoding

### 4.1. Change Types

When encoding a delta from `baseSnapshot` to `currentSnapshot`, each entity falls into one of three categories:

1. **Created**: present in current but not in base.
2. **Deleted**: present in base but not in current.
3. **Updated/Unchanged**: present in both, but some fields may have changed.

To encode deltas compactly, we use **per-entity opcodes**:

```cpp
enum class EntityDeltaOp : std::uint8_t {
    Create  = 0,
    Update  = 1,
    Delete  = 2
};
```

### 4.2. Delta Payload Layout (on the wire)

The `WorldSnapshot` payload begins with a small header, followed by a sequence of entity deltas:

```cpp
struct WorldSnapshotDeltaHeader {
    std::uint32_t snapshotId;     // ID of the current snapshot
    std::uint32_t baseSnapshotId; // ID of the base snapshot (0xFFFFFFFF if none)
    std::uint32_t serverTick;     // Tick at which snapshot was taken
    std::uint16_t entityDeltaCount; // Number of entity deltas that follow
    // followed by entityDeltaCount deltas
};
```

For each entity delta:

```text
[ EntityDeltaOp (uint8) ]
[ entityId (uint32) ]
[ ... additional data depending on op ... ]
```

- For **Create**:
  - We must send the full `EntityNetState`.
- For **Delete**:
  - Only `entityId` is needed.
- For **Update**:
  - We encode only the changed fields using a bitmask.

#### 4.2.1. Create encoding

```text
uint8   op          // EntityDeltaOp::Create
uint32  entityId
uint16  type
int16   x
int16   y
int16   vx
int16   vy
uint8   hp
uint8   flags
```

#### 4.2.2. Delete encoding

```text
uint8   op          // EntityDeltaOp::Delete
uint32  entityId
```

#### 4.2.3. Update encoding

We use a bitmask to indicate which fields changed:

```cpp
enum EntityFieldMask : std::uint8_t {
    Field_Type  = 1 << 0,
    Field_X     = 1 << 1,
    Field_Y     = 1 << 2,
    Field_Vx    = 1 << 3,
    Field_Vy    = 1 << 4,
    Field_Hp    = 1 << 5,
    Field_Flags = 1 << 6
};
```

On the wire:

```text
uint8   op          // EntityDeltaOp::Update
uint32  entityId
uint8   fieldMask

[ if fieldMask & Field_Type  ] uint16 type
[ if fieldMask & Field_X     ] int16  x
[ if fieldMask & Field_Y     ] int16  y
[ if fieldMask & Field_Vx    ] int16  vx
[ if fieldMask & Field_Vy    ] int16  vy
[ if fieldMask & Field_Hp    ] uint8  hp
[ if fieldMask & Field_Flags ] uint8  flags
```

### 4.3. Full Snapshot vs Delta Snapshot

- If `baseSnapshotId == 0xFFFFFFFF`:
  - The snapshot is **full**: the delta contains only `Create` operations for all entities.
- If `baseSnapshotId` refers to an existing snapshot on the client:
  - The payload is a **delta**:
    - For entities in base but not in current → `Delete`.
    - For entities in current but not in base → `Create`.
    - For entities in both → `Update` if any field changed, otherwise no entry.

The server is free to decide when to send a full snapshot instead of a delta, for example:

- On first snapshot for a client.
- Periodically (e.g., every N snapshots) as a correction against long-term drift.
- When the base snapshot is too old or missing in history.

---

## 5. Delta Encoding Algorithm (Server-Side)

Given:

- `baseSnapshot` (may be null, for full snapshots).
- `currentSnapshot`.

Server-side pseudo-code:

```cpp
WorldSnapshotDeltaHeader header;
header.snapshotId     = currentSnapshot.snapshotId;
header.baseSnapshotId = baseSnapshot ? baseSnapshot->snapshotId : 0xFFFFFFFFu;
header.serverTick     = currentSnapshot.serverTick;

// We'll fill entityDeltaCount after encoding.
std::vector<EntityDelta> deltas;

// Build a map from entityId to EntityNetState for both snapshots.
auto baseMap    = makeEntityMap(baseSnapshot);    // entityId -> EntityNetState*
auto currentMap = makeEntityMap(&currentSnapshot);

// 1) Handle created or updated entities
for (const auto& [entityId, currentState] : currentMap) {
    auto itBase = baseMap.find(entityId);
    if (itBase == baseMap.end()) {
        // Entity is new -> Create
        deltas.push_back(encodeCreate(currentState));
    } else {
        const EntityNetState& baseState = *itBase->second;
        auto maybeUpdate = encodeUpdateIfChanged(baseState, currentState);
        if (maybeUpdate.has_value()) {
            deltas.push_back(*maybeUpdate);
        }
        baseMap.erase(itBase); // remaining entries in baseMap will be "deleted"
    }
}

// 2) Remaining entities in baseMap are deleted
for (const auto& [entityId, baseState] : baseMap) {
    deltas.push_back(encodeDelete(entityId));
}

// 3) Write header.entityDeltaCount = deltas.size()
```

`encodeUpdateIfChanged` computes the field mask, and if the mask is zero (no difference), it returns `std::nullopt`.

---

## 6. Client-Side Snapshot Reconstruction

### 6.1. Snapshot Buffer

The client maintains a history buffer similar to the server’s:

```cpp
static constexpr std::size_t kMaxClientSnapshots = 64;

struct ClientSnapshot {
    std::uint32_t snapshotId;
    std::uint32_t serverTick;
    std::vector<EntityNetState> entities;
};

class ClientSnapshotHistory {
public:
    const ClientSnapshot* getSnapshot(std::uint32_t snapshotId) const;
    ClientSnapshot&       addSnapshot(std::uint32_t snapshotId, std::uint32_t serverTick);

private:
    ClientSnapshot _buffer[kMaxClientSnapshots];
    std::size_t    _head = 0;
};
```

### 6.2. Applying a Delta

When the client receives a `WorldSnapshot` payload:

1. Read `snapshotId`, `baseSnapshotId`, `serverTick`, `entityDeltaCount`.
2. If `baseSnapshotId == 0xFFFFFFFF`:
   - Create a new snapshot with an empty entity list and apply only `Create` deltas.
3. Else:
   - Look up the base snapshot in `ClientSnapshotHistory`.
   - If found:
     - Copy the base snapshot into a new `ClientSnapshot` object.
     - Apply deltas (Create/Update/Delete) to build the new snapshot.
   - If not found:
     - The delta cannot be applied safely.
     - Strategy options:
       - Request a full snapshot (out-of-scope here, optional).
       - Drop this snapshot and wait for the next; server logic may occasionally send full snapshots.

Pseudo-code for applying a delta:

```cpp
ClientSnapshot newSnap;
newSnap.snapshotId = header.snapshotId;
newSnap.serverTick = header.serverTick;

if (header.baseSnapshotId == 0xFFFFFFFFu) {
    // Start from empty
    newSnap.entities.clear();
} else {
    const ClientSnapshot* baseSnap = history.getSnapshot(header.baseSnapshotId);
    if (!baseSnap) {
        // Cannot apply delta safely; drop or handle via fallback.
        return;
    }
    newSnap.entities = baseSnap->entities; // copy base state
}

// Turn entities vector into a map for efficient operations
auto entityMap = makeEntityMap(newSnap.entities);

for each encoded delta in payload:
    switch (delta.op) {
    case EntityDeltaOp::Create:
        entityMap[delta.entityId] = decodeCreate(delta);
        break;
    case EntityDeltaOp::Delete:
        entityMap.erase(delta.entityId);
        break;
    case EntityDeltaOp::Update:
        applyUpdate(entityMap[delta.entityId], delta);
        break;
    }

// Rebuild entities vector from map
newSnap.entities = flattenEntityMap(entityMap);

// Store in history
history.addSnapshot(newSnap);
```

---

## 7. Interpolation and Extrapolation (Client)

Once the client has a history of full snapshots, the rendering code can:

1. Maintain a **render time** that lags behind `serverTick` by some interpolation offset (e.g., 50–100 ms).
2. For each frame:
   - Find two snapshots that bracket the render time:
     - `snapA` at `tickA`, `snapB` at `tickB`.
   - For each visible entity:
     - If present in both snapshots:
       - Interpolate position/velocity between `snapA` and `snapB`.
     - If present only in one snapshot:
       - Use the existing snapshot or fade the entity in/out.

Interpolation for position:

```cpp
float alpha = (renderTime - tickA) / float(tickB - tickA);
interpX = lerp(snapA.x, snapB.x, alpha);
interpY = lerp(snapA.y, snapB.y, alpha);
```

### 7.1. Extrapolation on Snapshot Loss

If the client does not receive a new snapshot for some time (due to packet loss or network hiccup):

- If there is only one valid snapshot for an entity:
  - The client may **extrapolate** using velocity for a short duration.
- After a maximum extrapolation time (e.g., 250–500 ms):
  - The entity can be frozen or hidden to avoid large visual errors.

The exact interpolation/extrapolation strategy belongs to the game client and rendering code, but the snapshot model is designed to support it cleanly.

---

## 8. Handling Snapshot Loss and Base Mismatch

If the server chooses `baseSnapshotId` based on the last acknowledged snapshot from the client, base mismatches should be rare. However, they can still happen, for example:

- Client dropped a full snapshot message.
- Client reset its snapshot history due to internal errors.

Possible strategies:

1. **Graceful drop** (simplest):  
   - If base snapshot not found on client, ignore this snapshot.
   - The next snapshot may be based on a newer base or be a full snapshot.
2. **Periodic full snapshots**:  
   - The server sends a full snapshot every N snapshots or after a timeout.
   - This bounds the time a client can be out-of-date.
3. **On-demand full snapshot** (more complex):  
   - Client can request a full snapshot if it detects repeated base mismatches.

For R-Type and similar small-scale arcade games, a combination of (1) and (2) is usually sufficient.

---

## 9. Summary

The snapshot and delta model for the R-Type UDP protocol is:

- **Server-authoritative**: server periodically captures full world state as snapshots.
- **Delta-compressed**: snapshots are sent as deltas from a base snapshot known by the client.
- **Entity-centric**: each entity is tracked by a stable `entityId` and compact `EntityNetState`.
- **Resilient to loss**: snapshots are unreliable; the system is designed so that newer snapshots always supersede older ones, and occasional losses can be hidden via interpolation/extrapolation.

This model provides a solid foundation for a fast-paced multiplayer shoot’em up, inspired by Quake3-style netcode but adapted to R-Type’s 2D, entity-based gameplay.
