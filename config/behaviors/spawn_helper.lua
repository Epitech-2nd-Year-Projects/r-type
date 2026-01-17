SpawnHelper = {}

function SpawnHelper.ApplyPlayerModifiers(registry, entity)
    if not DifficultyModifiers then 
        return 
    end
    
    local health = registry:get_health(entity)
    if health then
        local player_health = DifficultyModifiers.player_health or 100
        health.current_health = player_health
        health.max_health = player_health
    end
    
    local player = registry:get_player(entity)
    if player then
        local player_lives = DifficultyModifiers.player_lives or 3
        player.lives = player_lives
    end
end

function SpawnHelper.ApplyEnemyModifiers(registry, entity)
    if not DifficultyModifiers then return end
    
    local health = registry:get_health(entity)
    if health then
        local mult = DifficultyModifiers.enemy_health_multiplier or 1.0
        health.current_health = math.floor(health.current_health * mult)
        health.max_health = health.current_health
    end

    local dmg = registry:get_damageable(entity)
    if dmg then
        local mult = DifficultyModifiers.enemy_damage_multiplier or 1.0
        dmg.damage = math.floor(dmg.damage * mult)
    end
end

function SpawnWithDifficulty(registry, prefab_name, x, y)
    local entity = Spawn(registry, prefab_name, x, y)
    if not entity then return nil end
    
    local tag = registry:get_tag(entity)
    if tag then
        if tag.tag == "Player" then
            SpawnHelper.ApplyPlayerModifiers(registry, entity)
        elseif tag.tag == "Enemy" then
            SpawnHelper.ApplyEnemyModifiers(registry, entity)
        end
    end
    
    return entity
end


function GetScoreMultiplier()
    if DifficultyModifiers and DifficultyModifiers.score_multiplier then
        return DifficultyModifiers.score_multiplier
    end
    return 1.0
end
