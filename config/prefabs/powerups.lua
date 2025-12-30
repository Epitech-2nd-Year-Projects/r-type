Prefabs = Prefabs or {}


Prefabs.PowerupDropTable = {
    { name = "HealthPotion", weight = 50 },
    { name = "WeaponUpgrade", weight = 30 },
    { name = "ScoreBonus", weight = 20 }
}

Prefabs.HealthPotion = {
    Tag = "Powerup",
    Velocity = { x = -50.0, y = 0.0 },
    BoundingBox = { width = 16.0, height = 16.0 },
    Sprite = {
        texture = "assets/sprites/powerup_green.png",
        width = 16.0,
        height = 16.0,
        layer = 5,
    },
    Powerup = {
        type = PowerupType.Health,
        value = 30
    }
}

Prefabs.WeaponUpgrade = {
    Tag = "Powerup",
    Velocity = { x = -50.0, y = 0.0 },
    BoundingBox = { width = 16.0, height = 16.0 },
    Sprite = {
        texture = "assets/sprites/powerup_red.png",
        width = 16.0,
        height = 16.0,
        layer = 5,
    },
    Powerup = {
        type = PowerupType.WeaponUpgrade,
        value = 1
    }
}

Prefabs.ScoreBonus = {
    Tag = "Powerup",
    Velocity = { x = -50.0, y = 0.0 },
    BoundingBox = { width = 16.0, height = 16.0 },
    Sprite = {
        texture = "assets/sprites/powerup_blue.png",
        width = 16.0,
        height = 16.0,
        layer = 5,
    },
    Powerup = {
        type = PowerupType.Score,
        value = 500
    }
}
