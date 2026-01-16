Prefabs = Prefabs or {}


Prefabs.PowerupDropTable = {
    { name = "HealthPotion", weight = 50 },
    { name = "WeaponUpgrade", weight = 20 },
    { name = "ScoreBonus", weight = 30 }
}

Prefabs.HealthPotion = {
    Tag = "Powerup",
    Velocity = { x = -50.0, y = 0.0 },
    BoundingBox = { width = 21.0, height = 21.0 },
    Sprite = {
        texture = "assets/sprites/Heart_Powerup.png",
        width = 22.0,
        height = 22.0,
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
    BoundingBox = { width = 22.0, height = 22.0 },
    Sprite = {
        texture = "assets/sprites/Weapon_Powerup.png",
        width = 22.0,
        height = 22.0,
        layer = 5,
    },
    Powerup = {
        type = PowerupType.WeaponUpgrade,
        value = 10
    }
}

Prefabs.ScoreBonus = {
    Tag = "Powerup",
    Velocity = { x = -50.0, y = 0.0 },
    BoundingBox = { width = 19.0, height = 19.0 },
    Sprite = {
        texture = "assets/sprites/Score_Powerup.png",
        width = 22.0,
        height = 22.0,
        layer = 5,
    },
    Powerup = {
        type = PowerupType.Score,
        value = 500
    }
}
