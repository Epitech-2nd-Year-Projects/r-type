Prefabs = Prefabs or {}
    
Prefabs.PlayerMissile = {
    Tag = "Missile",
    Damageable = {
        damage = 10,
        faction = 0,
        friendly_fire = false,
    },
    Velocity = { x = 0, y = 0 },
    Lifetime = 5.0,
    BoundingBox = { width = 12.8, height = 6.4 },
    Sprite = {
        texture = "assets/sprites/player_missile.png",
        width = 16,
        height = 8,
        layer = 8,
        tint = { r = 100, g = 150, b = 255, a = 255 }
    }
}

Prefabs.BigPlayerMissile = {
    Tag = "Missile",
    Damageable = {
        damage = 50,
        faction = 0,
        friendly_fire = false,
    },
    Velocity = { x = 0, y = 0 },
    Lifetime = 5.0,
    BoundingBox = { width = 25.6, height = 12.8 },
    Sprite = {
        texture = "assets/sprites/big_missile.png",
        width = 32,
        height = 16,
        layer = 8,
        tint = { r = 255, g = 255, b = 0, a = 255 }
    }
}

Prefabs.EnemyMissile = {
    Tag = "Missile",
    Damageable = {
        damage = 35,
        faction = 1,
        friendly_fire = false,
    },
    Velocity = { x = 0, y = 0 },
    Lifetime = 5.0,
    BoundingBox = { width = 9.6, height = 9.6 },
    Sprite = {
        texture = "assets/sprites/enemy_missile.png",
        width = 12,
        height = 12,
        layer = 8,
        tint = { r = 255, g = 100, b = 100, a = 255 }
    }
}

Prefabs.NeutralMissile = {
    Tag = "Missile",
    Damageable = {
        damage = 20,
        faction = 2,
        friendly_fire = false,
    },
    Velocity = { x = 0, y = 0 },
    Lifetime = 5.0,
    BoundingBox = { width = 11.2, height = 8.0 }, 
    Sprite = {
        texture = "assets/sprites/neutral_missile.png",
        width = 14,
        height = 10,
        layer = 8,
        tint = { r = 200, g = 200, b = 200, a = 255 }
    }
}
