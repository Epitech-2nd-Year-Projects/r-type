local INSTANT_KILL_DAMAGE = 9999

DifficultySettings = {
    Easy = {
        enemy_speed_multiplier = 0.7,
        enemy_health_multiplier = 0.75,
        enemy_damage_multiplier = 0.5,
        enemy_fire_rate_multiplier = 0.6,
        player_health = 150,
        player_lives = 5,
        score_multiplier = 0.75,
    },
    
    Normal = {
        enemy_speed_multiplier = 1.0,
        enemy_health_multiplier = 1.0,
        enemy_damage_multiplier = 1.0,
        enemy_fire_rate_multiplier = 1.0,
        player_health = 100,
        player_lives = 3,
        score_multiplier = 1.0,
    },
    
    Hard = {
        enemy_speed_multiplier = 1.3,
        enemy_health_multiplier = 1.5,
        enemy_damage_multiplier = 1.5,
        enemy_fire_rate_multiplier = 1.5,
        player_health = 80,
        player_lives = 2,
        score_multiplier = 1.5,
    },
    
    Hardcore = {
        enemy_speed_multiplier = 1.5,
        enemy_health_multiplier = 2.0,
        enemy_damage_multiplier = INSTANT_KILL_DAMAGE,
        enemy_fire_rate_multiplier = 2.0,
        player_health = 1,
        player_lives = 1,
        score_multiplier = 2.0,
    }
}

function GetDifficultyModifiers(difficulty_name)
    return DifficultySettings[difficulty_name] or DifficultySettings.Normal
end

DifficultyNames = {
    [0] = "Easy",
    [1] = "Normal",
    [2] = "Hard",
    [3] = "Hardcore"
}
