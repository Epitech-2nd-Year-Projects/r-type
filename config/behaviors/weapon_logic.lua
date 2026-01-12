WeaponLogic = WeaponLogic or {}

function WeaponLogic.BasicPlayerWeapon(entity_id, dt, weapon, position)
    if weapon.cooldown_remaining > 0 then
        weapon.cooldown_remaining = weapon.cooldown_remaining - dt
    end

    if weapon.big_shot_cooldown_remaining > 0 then
        weapon.big_shot_cooldown_remaining = weapon.big_shot_cooldown_remaining - dt
    end

    if weapon.is_big_trigger_held and weapon.big_shot_cooldown_remaining <= 0 then
        weapon:fire_big(1.0)
        local big_prefab = "BigPlayerMissile"
        
        local spawn_x = position.position.x + 16.0
        local spawn_y = position.position.y
        
        local entity = Spawn(registry, big_prefab, spawn_x, spawn_y)
        if entity then
             local speed = 250.0
             registry:add_velocity(entity, speed, 0.0)
        end
    end

    if weapon.is_trigger_held and weapon.cooldown_remaining <= 0 and weapon:can_fire() then
        if weapon.fire_rate > 0 then
             weapon:fire(weapon.fire_rate)
        else
             weapon:fire(2.0)
        end
        
        local prefab_name = weapon.projectile_prefab
        if prefab_name == "" then prefab_name = "PlayerMissile" end
        
        local spawn_x = position.position.x + 16.0
        local spawn_y = position.position.y
        
        local entity = Spawn(registry, prefab_name, spawn_x, spawn_y)
        
        if entity then
             local speed = weapon.projectile_speed
             if speed <= 0 then speed = 400.0 end
             
             registry:add_velocity(entity, speed, 0.0)
        end
    end
end

function WeaponLogic.BasicEnemyWeapon(entity_id, dt, weapon, position)
    if weapon.cooldown_remaining > 0 then
        weapon.cooldown_remaining = weapon.cooldown_remaining - dt
    end
    


    if weapon.is_trigger_held and weapon.cooldown_remaining <= 0 then
        if weapon.fire_rate > 0 then
             weapon:fire(weapon.fire_rate)
        else
             weapon:fire(0.5)
        end
        
        local prefab_name = weapon.projectile_prefab
        if prefab_name == "" then prefab_name = "EnemyMissile" end
        
        local spawn_x = position.position.x - 16.0
        local spawn_y = position.position.y
        
        local entity = Spawn(registry, prefab_name, spawn_x, spawn_y)
        if entity then
             local speed = weapon.projectile_speed
             if speed <= 0 then speed = 300.0 end
             
             registry:add_velocity(entity, -speed, 0.0)
        end
    end
end

function WeaponLogic.DobkeratopsWeapon(entity_id, dt, weapon, position)
    if weapon.cooldown_remaining > 0 then
        weapon.cooldown_remaining = weapon.cooldown_remaining - dt
    end
    
    if weapon.is_trigger_held and weapon.cooldown_remaining <= 0 then
        weapon:fire(weapon.fire_rate)
        
        local prefab_name = weapon.projectile_prefab
        if prefab_name == "" then prefab_name = "EnemyMissile" end

        local num_projectiles = weapon.projectiles_per_burst or 5
        local total_spread = 0.6
        
        for i = 1, num_projectiles do
            local angle = 0
            if num_projectiles > 1 then
                angle = -total_spread / 2 + (i - 1) * (total_spread / (num_projectiles - 1))
            end
            
            local spawn_x = position.position.x - 40.0
            local spawn_y = position.position.y + 50.0
            
            local entity = Spawn(registry, prefab_name, spawn_x, spawn_y)
            if entity then
                local speed = weapon.projectile_speed
                if speed <= 0 then speed = 300.0 end
                
                local vx = -speed * math.cos(angle)
                local vy = speed * math.sin(angle)
                
                registry:add_velocity(entity, vx, vy)
            end
        end
    end
end
