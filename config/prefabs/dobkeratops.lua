Prefabs = Prefabs or {}

Prefabs.Dobkeratops = {
    Tag = "Enemy",
    Health = 500,
    ScoreValue = 5000,
    Velocity = { x = -50, y = 0 },
    Sprite = {
        texture = "assets/sprites/Dobkeratops.png",
        width = 162,
        height = 123,
        layer = 3,
        scale = 2.0
    },
    BoundingBox = { width = 165, height = 200 },
    AI = {
        behavior = "Dobkeratops",
        speed = 50,
        entry_speed = 100,
        stop_x = 600,
        oscillate_speed = 1.0,
        oscillate_amp = 100
    },
    Weapon = {
        projectile_name = "EnemyMissile",
        projectile_speed = 300,
        fire_rate = 1.0,
        damage = 50,
        faction = 1,
        trigger_held = true,
        weapon_script = "DobkeratopsWeapon",
        projectiles_per_burst = 5,
    }
}
