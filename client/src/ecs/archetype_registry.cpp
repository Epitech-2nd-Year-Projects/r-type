#include "ecs/archetype_registry.h"

#include <array>
#include <cmath>
#include <string_view>

namespace client::ecs {

namespace {

constexpr std::uint16_t kPlayerTypeCode = 1u;
constexpr std::uint16_t kEnemyTypeCode = 2u;
constexpr std::uint16_t kMissileTypeCode = 3u;
constexpr std::uint16_t kObstacleTypeCode = 4u;
constexpr std::uint16_t kPowerupTypeCode = 5u;

constexpr std::int32_t kPlayerRenderLayer = 10;
constexpr std::int32_t kEnemyRenderLayer = 5;
constexpr std::int32_t kMissileRenderLayer = 8;
constexpr std::int32_t kObstacleRenderLayer = 3;

constexpr float kEnemySpriteSize = 29.0f;
constexpr float kPlayerSpriteWidth = 26.0f;
constexpr float kPlayerSpriteHeight = 21.0f;
constexpr float kMissileSpriteWidth = 19.0f;
constexpr float kMissileSpriteHeight = 6.0f;
constexpr float kPowerupSpriteWidth = 16.0f;
constexpr float kPowerupSpriteHeight = 16.0f;
constexpr float kDefaultObstacleSize = 64.0f;

constexpr float kInterceptorSpeedThreshold = 170.0f;
constexpr float kInterceptorVerticalThreshold = 15.0f;

constexpr std::uint32_t kTankHealthThreshold = 150u;
constexpr std::uint32_t kBomberHealthThreshold = 20u;

const std::string_view kPlayerTexture = "assets/sprites/player.png";
const std::string_view kEnemyScoutTexture = "assets/sprites/enemy_scout.png";
const std::string_view kEnemyBomberTexture = "assets/sprites/enemy_bomber.png";
const std::string_view kEnemyTankTexture = "assets/sprites/enemy_tank.png";
const std::string_view kEnemyInterceptorTexture =
    "assets/sprites/enemy_interceptor.png";
const std::string_view kPlayerMissileTexture =
    "assets/sprites/player_missile.png";
const std::string_view kEnemyMissileTexture =
    "assets/sprites/enemy_missile.png";
const std::string_view kBigMissileTexture = "assets/sprites/big_missile.png";
const std::string_view kNeutralMissileTexture =
    "assets/sprites/neutral_missile.png";
const std::string_view kWallTexture = "assets/sprites/obstacle_wall.png";
const std::string_view kBarrierTexture =
    "assets/sprites/obstacle_destructible.png";
const std::string_view kDobkeratopsTexture = "assets/sprites/Dobkeratops.png";
const std::string_view kPowerupTexture = "assets/sprites/powerup_green.png";

SpriteDefinition MakeDefinition(
    std::string_view texture, float width, float height, std::int32_t layer,
    float depth, bool face_left,
    engine::render::Color tint = engine::render::Color::White()) {
  SpriteDefinition def{};
  def.texture_id = std::string(texture);
  def.source_rect = engine::math::RectF(0.0f, 0.0f, width, height);
  def.layer = layer;
  def.depth = depth;
  def.face_left = face_left;
  def.tint = tint;
  return def;
}

SpriteDefinition PlayerDefinition(std::uint32_t player_id) {
  if (player_id < 4) {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    switch (player_id) {
      case 0:
        r = 0;
        g = 255;
        b = 255;
        break;
      case 1:
        r = 255;
        g = 60;
        b = 60;
        break;
      case 2:
        r = 60;
        g = 255;
        b = 60;
        break;
      case 3:
        r = 255;
        g = 255;
        b = 0;
        break;
    }
    return MakeDefinition(kPlayerTexture, kPlayerSpriteWidth,
                          kPlayerSpriteHeight, kPlayerRenderLayer, 0.0f, false,
                          engine::render::Color::FromBytes(r, g, b));
  }

  std::uint32_t hash = player_id * 2654435761u;
  std::uint8_t r = static_cast<std::uint8_t>(128 + ((hash >> 0) & 0x7F));
  std::uint8_t g = static_cast<std::uint8_t>(128 + ((hash >> 8) & 0x7F));
  std::uint8_t b = static_cast<std::uint8_t>(128 + ((hash >> 16) & 0x7F));
  return MakeDefinition(kPlayerTexture, kPlayerSpriteWidth, kPlayerSpriteHeight,
                        kPlayerRenderLayer, 0.0f, false,
                        engine::render::Color::FromBytes(r, g, b));
}

SpriteDefinition EnemyDefinition(std::string_view texture, float depth) {
  return MakeDefinition(texture, kEnemySpriteSize, kEnemySpriteSize,
                        kEnemyRenderLayer, depth, true);
}

SpriteDefinition BossDefinition(std::string_view texture, float width,
                                float height, float depth) {
  return MakeDefinition(texture, width, height, kEnemyRenderLayer, depth,
                        false);
}

SpriteDefinition MissileDefinition(std::string_view texture, bool face_left) {
  return MakeDefinition(texture, kMissileSpriteWidth, kMissileSpriteHeight,
                        kMissileRenderLayer, 0.0f, face_left);
}

SpriteDefinition ObstacleDefinition(std::string_view texture, float scale) {
  const float size = kDefaultObstacleSize * scale;
  return MakeDefinition(texture, size, size, kObstacleRenderLayer, 0.0f, false);
}

std::optional<SpriteDefinition> ResolveEnemySprite(
    const SpriteContext &context) {
  const auto hp = context.health.has_value()
                      ? static_cast<std::uint32_t>(context.health->max)
                      : 0u;
  const float speed =
      context.velocity.has_value() ? context.velocity->velocity.Length() : 0.0f;

  if (hp >= 500u) {
    return BossDefinition(kDobkeratopsTexture, 165.0f, 200.0f, 0.4f);
  }
  if (hp >= kTankHealthThreshold) {
    return EnemyDefinition(kEnemyTankTexture, 0.3f);
  }
  if (hp >= kBomberHealthThreshold) {
    return EnemyDefinition(kEnemyBomberTexture, 0.2f);
  }
  if (speed > kInterceptorSpeedThreshold ||
      (context.velocity.has_value() && std::abs(context.velocity->velocity.y) >
                                           kInterceptorVerticalThreshold)) {
    return EnemyDefinition(kEnemyInterceptorTexture, 0.1f);
  }

  if (context.entity_index % 2 == 1) {
    return EnemyDefinition(kEnemyInterceptorTexture, 0.1f);
  }
  return EnemyDefinition(kEnemyScoutTexture, 0.0f);
}

std::optional<SpriteDefinition> ResolveMissileSprite(
    const SpriteContext &context) {
  const float vx =
      context.velocity.has_value() ? context.velocity->velocity.x : 1.0f;
  const float speed =
      context.velocity.has_value() ? context.velocity->velocity.Length() : 0.0f;

  if (vx < -5.0f) {
    return MissileDefinition(kEnemyMissileTexture, true);
  }
  if (speed > 260.0f) {
    return MissileDefinition(kPlayerMissileTexture, false);
  }
  if (speed > 0.0f && speed < 220.0f) {
    return MissileDefinition(kNeutralMissileTexture, vx < 0.0f);
  }
  return MissileDefinition(kBigMissileTexture, false);
}

std::optional<SpriteDefinition> ResolveObstacleSprite(
    const SpriteContext &context) {
  if (context.health.has_value() && context.health->max == 0u) {
    return ObstacleDefinition(kWallTexture, 1.0f);
  }
  return ObstacleDefinition(kBarrierTexture, 1.0f);
}

std::optional<SpriteDefinition> ResolvePowerupSprite() {
  return MakeDefinition(kPowerupTexture, kPowerupSpriteWidth,
                        kPowerupSpriteHeight, kEnemyRenderLayer, 0.5f, false);
}

const std::array<ArchetypeDefinition, 5> &ArchetypeDefinitions() {
  static const std::array<ArchetypeDefinition, 5> definitions = {
      ArchetypeDefinition{kPlayerTypeCode, ArchetypeKind::kPlayer, true, false},
      ArchetypeDefinition{kEnemyTypeCode, ArchetypeKind::kEnemy, true, false},
      ArchetypeDefinition{kMissileTypeCode, ArchetypeKind::kMissile, false,
                          true},
      ArchetypeDefinition{kObstacleTypeCode, ArchetypeKind::kObstacle, true,
                          false},
      ArchetypeDefinition{kPowerupTypeCode, ArchetypeKind::kPowerup, false,
                          false}};
  return definitions;
}

}  // namespace

const ArchetypeRegistry &ArchetypeRegistry::Get() {
  static const ArchetypeRegistry registry;
  return registry;
}

const ArchetypeDefinition *ArchetypeRegistry::Find(
    std::uint16_t type_code) const {
  const auto &definitions = ArchetypeDefinitions();
  for (const auto &def : definitions) {
    if (def.type_code == type_code) {
      return &def;
    }
  }
  return nullptr;
}

ArchetypeKind ArchetypeRegistry::KindOf(std::uint16_t type_code) const {
  const auto *def = Find(type_code);
  return def ? def->kind : ArchetypeKind::kUnknown;
}

bool ArchetypeRegistry::IsKind(std::uint16_t type_code,
                               ArchetypeKind kind) const {
  const auto *def = Find(type_code);
  return def && def->kind == kind;
}

bool ArchetypeRegistry::IsPlayer(std::uint16_t type_code) const {
  return IsKind(type_code, ArchetypeKind::kPlayer);
}

bool ArchetypeRegistry::IsEnemy(std::uint16_t type_code) const {
  return IsKind(type_code, ArchetypeKind::kEnemy);
}

bool ArchetypeRegistry::IsMissile(std::uint16_t type_code) const {
  return IsKind(type_code, ArchetypeKind::kMissile);
}

bool ArchetypeRegistry::IsObstacle(std::uint16_t type_code) const {
  return IsKind(type_code, ArchetypeKind::kObstacle);
}

bool ArchetypeRegistry::IsPowerup(std::uint16_t type_code) const {
  return IsKind(type_code, ArchetypeKind::kPowerup);
}

bool ArchetypeRegistry::IsDamageable(std::uint16_t type_code) const {
  const auto *def = Find(type_code);
  return def && def->damageable;
}

bool ArchetypeRegistry::IsExplosive(std::uint16_t type_code) const {
  const auto *def = Find(type_code);
  return def && def->explosive;
}

std::optional<SpriteDefinition> ArchetypeRegistry::ResolveSprite(
    std::uint16_t type_code, const SpriteContext &context) const {
  switch (KindOf(type_code)) {
    case ArchetypeKind::kPlayer:
      return PlayerDefinition(context.network_id);
    case ArchetypeKind::kEnemy:
      return ResolveEnemySprite(context);
    case ArchetypeKind::kMissile:
      return ResolveMissileSprite(context);
    case ArchetypeKind::kObstacle:
      return ResolveObstacleSprite(context);
    case ArchetypeKind::kPowerup:
      return ResolvePowerupSprite();
    case ArchetypeKind::kUnknown:
    default:
      return std::nullopt;
  }
}

}  // namespace client::ecs
