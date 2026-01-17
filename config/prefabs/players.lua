Prefabs = Prefabs or {}

Prefabs.Player = {
    Tag = "Player",
    Health = { max_health = 100 },
    Velocity = { x = 0, y = 0 },
    BoundingBox = { width = 36, height = 18 },
    Sprite = {
        texture = "assets/sprites/player.png",
        width = 36,
        height = 18,
        frame_width = 26,
        frame_height = 21,
        layer = 10,
    },
    Weapon = {
        projectile_name = "PlayerMissile", 
        projectile_speed = 400.0,
        fire_rate = 2.0,
        damage = 10,
        big_projectile_name = "BigPlayerMissile",
        big_projectile_speed = 250.0,
        weapon_script = "BasicPlayerWeapon",
    },
    PlayerValue = {
        lives = 3,
        score = 0
    }
}
