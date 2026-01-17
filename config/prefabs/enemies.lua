Prefabs = Prefabs or {}

Prefabs.Scout = {
    Tag = "Enemy",
    Health = 10,
    ScoreValue = 100,
    Velocity = { x = -150, y = 0 },
    Sprite = {
        texture = "assets/sprites/enemy_scout.png",
        width = 36,
        height = 36,
        frame_width = 33,
        frame_height = 33,
        layer = 2,
    },
    BoundingBox = { width = 36, height = 36 },
    AI = {
        behavior = "Straight",
        speed = 150,
        detection_range = 0,
    },
    Weapon = {
        projectile_name = "EnemyMissile",
        projectile_speed = 300,
        fire_rate = 0.5,
        damage = 35,
        faction = 1,
        trigger_held = true,
        weapon_script = "BasicEnemyWeapon",
    }
}

Prefabs.Bomber = {
    Tag = "Enemy",
    Health = 20,
    ScoreValue = 200,
    Velocity = { x = -100, y = 0 },
    Sprite = {
        texture = "assets/sprites/enemy_bomber.png",
        width = 36,
        height = 36,
        frame_width = 33,
        frame_height = 33,
        layer = 2,
    },
    BoundingBox = { width = 36, height = 36 },
    AI = {
        behavior = "WavePattern",
        speed = 100,
        wave_amplitude = 50,
        wave_frequency = 2,
    }
}

Prefabs.Tank = {
    Tag = "Enemy",
    Health = 150,
    ScoreValue = 500,
    Velocity = { x = -50, y = 0 },
    Sprite = {
        texture = "assets/sprites/enemy_tank.png",
        width = 36,
        height = 36,
        frame_width = 33,
        frame_height = 33,
        layer = 2,
    },
    BoundingBox = { width = 36, height = 36 },
    AI = {
        behavior = "Patrol",
        speed = 50,
    },
    Weapon = {
        projectile_name = "EnemyMissile",
        projectile_speed = 300,
        fire_rate = 0.5,
        damage = 35,
        faction = 1,
        trigger_held = true,
        weapon_script = "BasicEnemyWeapon",
    }
}

Prefabs.Interceptor = {
    Tag = "Enemy",
    Health = 10,
    ScoreValue = 300,
    Velocity = { x = -150, y = 0 },
    Sprite = {
        texture = "assets/sprites/enemy_interceptor.png",
        width = 36,
        height = 36,
        frame_width = 33,
        frame_height = 33,
        layer = 2,
    },
    BoundingBox = { width = 36, height = 36 },
    AI = {
        behavior = "ChasePlayer",
        speed = 150,
        detection_range = 2000,
    }
}
