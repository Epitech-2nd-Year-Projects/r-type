Prefabs = Prefabs or {}

Prefabs.Wall = {
    Tag = "Obstacle",
    BoundingBox = { width = 64.0, height = 64.0 }, 
    Sprite = {
        texture = "assets/sprites/obstacle_wall.png",
        width = 64.0,
        height = 64.0,
        layer = 3,
        tint = { r = 128, g = 128, b = 128, a = 255 }
    }
}

Prefabs.DestructibleBarrier = {
    Tag = "Obstacle",
    Damageable = {
        damage = 0,
        faction = 2,
        friendly_fire = false,
    },
    Health = {
        max_health = 100,
        current_health = 100,
        invulnerable = false
    },
    ScoreValue = 50,
    BoundingBox = { width = 64.0, height = 64.0 },
    Sprite = {
        texture = "assets/sprites/obstacle_destructible.png",
        width = 64.0,
        height = 64.0,
        layer = 3,
        tint = { r = 160, g = 120, b = 80, a = 255 }
    }
}
