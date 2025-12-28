Prefabs = Prefabs or {}

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
        type = "Health",
        value = 30
    }
}
