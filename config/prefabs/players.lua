Prefabs = Prefabs or {}

Prefabs.Player = {
    Tag = "Player",
    Health = { max_health = 100 },
    Velocity = { x = 0, y = 0 },
    BoundingBox = { width = 32, height = 16 },
    Sprite = {
        texture = "assets/sprites/player.png",
        width = 32,
        height = 16,
        layer = 10,
    },
    Weapon = {
        projectile_name = "PlayerMissile", 
        projectile_speed = 400.0,
        fire_rate = 2.0,
        damage = 10,
        big_projectile_name = "BigPlayerMissile",
        big_projectile_speed = 250.0,
    },
    PlayerValue = {
        lives = 3,
        score = 0
    }
}
