# Rift - Feature specification

Rift is a 1v1 multiplayer fighting game designed to demonstrate the engine's game-agnostic architecture by reusing the same engine that powers R-Type.

## Game overview

- **Genre**: 1v1 multiplayer fighting game
- **Perspective**: Third-person with 2D-style combat (fighters face each other on a plane, like Tekken)
- **Platform**: PC (Windows/Linux)
- **Network**: Real-time multiplayer over UDP

## Controls

### Input scheme (4 buttons + movement)

| Input | Action |
|-------|--------|
| Left/Right | Move along the fight axis |
| Light Attack | Fast attack, low damage, quick recovery |
| Heavy Attack | Slow attack, high damage, long recovery |
| Block | Hold to reduce incoming damage (costs stamina) |
| Dodge | Quick invulnerable dash (costs stamina) |

## Core mechanics

### Health system
- Each fighter has 100 HP
- Taking damage reduces HP
- Round ends when a fighter's HP reaches 0
- HP resets to full at the start of each round

### Stamina system
- Each fighter has 100 stamina
- Stamina regenerates over time when not performing actions
- Stamina costs:
  - Light Attack: Low cost
  - Heavy Attack: Medium cost
  - Block (while holding): Drains over time
  - Dodge: High cost
- Actions cannot be performed if stamina is insufficient

### Combat states
Fighters can be in one of the following states:

| State | Description |
|-------|-------------|
| Idle | Standing, can perform any action |
| Walking | Moving left or right |
| Attacking | In startup, active, or recovery frames of an attack |
| Blocking | Reducing incoming damage |
| Stunned | Hit by an attack, temporarily unable to act |
| Dodging | Invulnerable, moving quickly |

### Attack properties
Each attack has frame data:
- **Startup frames**: Wind-up before the attack becomes active
- **Active frames**: Attack can hit opponent
- **Recovery frames**: Cooldown after attack, vulnerable

| Attack | Startup | Active | Recovery | Damage |
|--------|---------|--------|----------|--------|
| Light | Fast | Short | Short | ~10 |
| Heavy | Slow | Medium | Long | ~25 |

### Blocking
- Reduces damage taken (e.g., 75% reduction)
- Consumes stamina while holding
- Guard break occurs if stamina depletes while blocking

### Dodging
- Grants invincibility frames (i-frames)
- Moves the fighter a short distance
- High stamina cost
- Short recovery after dodge ends

## Match structure

### Format
- **Best of 3 rounds**
- First player to win 2 rounds wins the match

### Round flow
1. Fighters spawn at opposite ends of the arena
2. 3-second countdown ("3... 2... 1... FIGHT!")
3. Combat until:
   - One fighter's HP reaches 0 (KO)
   - 60-second timer expires (timeout - most HP wins)
4. Round winner announced
5. Positions and HP reset
6. Next round begins (or match ends if someone has 2 wins)

### Victory conditions
- **KO**: Reduce opponent's HP to 0
- **Timeout**: Have more HP when the timer expires
- **Match Win**: Win 2 rounds

## Network architecture

### Reused protocol library
The shared `protocol` library is reused without modification:
- UDP transport layer
- Reliability system for critical events
- Header structure with sequence/ack
- Ping/pong latency measurement
- Packet fragmentation
- Input state structure (button flags)
- World snapshot with entity states
- Command payloads

### Input mapping
The existing `InputButton` flags are mapped to fighting game actions:
- Movement buttons → Left/Right on fight axis
- Fire button → Light Attack
- BigFire button → Heavy Attack
- Additional buttons for Block/Dodge (extend if needed)

### Entity state
Uses `EntityNetState` from protocol with fighting-game interpretation:
- `type` → Fighter entity type
- `position` → 3D position (x = fight axis, z = depth)
- `health` → Fighter HP (0-100)
- `flags` → Combat state, facing direction, invulnerability

### Server architecture
- Server-authoritative simulation
- ~60 tick server rate
- Input processing with sequence tracking
- State snapshot broadcast

### Client architecture
- Input prediction
- State interpolation
- Rollback on correction (optional, for polish)

## Technical components

### Game logic components
| Component | Purpose |
|-----------|---------|
| FighterComponent | Player ID, slot (P1/P2), rounds won |
| HealthComponent | Current/max HP |
| StaminaComponent | Current/max stamina, regen rate |
| CombatStateComponent | Current state enum |
| AttackComponent | Active attack data and frame tracking |
| HurtboxComponent | Body collision volume |
| FighterMovementComponent | Position, facing, speed |

### Game logic systems
| System | Purpose |
|--------|---------|
| FighterInputSystem | Process player inputs |
| FighterMovementSystem | Handle movement on fight axis |
| CombatStateSystem | State machine transitions |
| AttackSystem | Attack hitbox lifecycle |
| HitDetectionSystem | Collision between hitbox and hurtbox |
| DamageSystem | Apply damage and stun |
| BlockSystem | Handle blocking logic |
| StaminaSystem | Regeneration and consumption |
| MatchStateSystem | Round/match flow |
| RiftStateSystem | Sync ECS to network snapshots |

## Engine reuse

The following engine systems are reused without modification:
- ECS (Registry, SparseArray, Zipper)
- EventBus
- Renderer3D, Camera3D
- AudioDispatcher
- InputManager
- ResourceManager
- GameRuntime (multi-threaded architecture)
- Networking primitives (UdpSocket, PacketBuffer)
- Math (Vector3f, Transform3D)

## Future considerations
- Character selection (multiple fighters with different stats)
- Combo system
- Super meter / special moves
- Ranked matchmaking
- Replay system
- Training mode
