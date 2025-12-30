function OnCollision(e1, e2)
    local tag1 = registry:get_tag(e1)
    local tag2 = registry:get_tag(e2)

    local function GetRandomPowerup()
        if not Prefabs or not Prefabs.PowerupDropTable then
             return "HealthPotion"
        end
        local total_weight = 0
        for _, entry in ipairs(Prefabs.PowerupDropTable) do
            total_weight = total_weight + (entry.weight or 0)
        end
        
        local roll = math.random() * total_weight
        local current = 0
        for _, entry in ipairs(Prefabs.PowerupDropTable) do
            current = current + (entry.weight or 0)
            if roll <= current then
                return entry.name
            end
        end
        return "HealthPotion"
    end

    local function HandlePowerupCollection(player_entity, powerup_entity)
        local p_comp = registry:get_powerup(powerup_entity)
        local hp_comp = registry:get_health(player_entity)
        local player_comp = registry:get_player(player_entity)
        
        if p_comp and p_comp.active then
            if p_comp.type == PowerupType.Health then
                 if hp_comp and hp_comp:is_alive() then
                      hp_comp.current_health = math.min(hp_comp.current_health + p_comp.value, hp_comp.max_health)
                      print("Powerup: Restored " .. p_comp.value .. " Health")
                 end
            elseif p_comp.type == PowerupType.Score then
                 if player_comp then
                      player_comp.score = player_comp.score + p_comp.value
                      print("Powerup: Bonus Score " .. p_comp.value)
                 end
            elseif p_comp.type == PowerupType.WeaponUpgrade then
                 print("Powerup: Weapon Upgrade Collected")
            end
            
            p_comp.active = false
            registry:kill_entity(powerup_entity)
        end
    end

    local function HandleDeath(entity, hp_comp)
        if hp_comp.current_health <= 0 then
             local player = registry:get_player(entity)
             if player then
                 if player.lives > 0 then
                     print("Player died! Lives remaining: " .. player.lives .. " -> " .. (player.lives - 1))
                     player.lives = player.lives - 1
                     hp_comp.current_health = hp_comp.max_health
                     
                     local pos_comp = registry:get_position(entity)
                     if pos_comp then
                        local slot = player.player_slot or 0
                        local respawn_x = 100.0 + (slot * 50.0)
                        local respawn_y = 300.0
                        registry:add_position(entity, respawn_x, respawn_y)
                     end

                     if SignalPlayerDeath then
                        SignalPlayerDeath(player.player_id, player.lives)
                     end
                     return
                 else
                     print("Player Game Over!")
                     if SignalPlayerDeath then
                        SignalPlayerDeath(player.player_id, 0)
                     end
                 end
             end

             local drops = registry:get_drops_powerup(entity)
             if drops then
                 local pos_comp = registry:get_position(entity)
                 if pos_comp then
                      local prefab_name = GetRandomPowerup()
                      Spawn(registry, prefab_name, pos_comp.position.x, pos_comp.position.y)
                 end
             end

             registry:kill_entity(entity)
        end
    end

    local function HandleCrash(victim, attacker)
        local hp = registry:get_health(victim)
        if hp and hp:is_alive() then
            hp:take_damage(100) 
            HandleDeath(victim, hp)
        end
    end

    local function HandleProjectile(proj_id, target_id)
        local dmg_comp = registry:get_damageable(proj_id)
        local target_tag = registry:get_tag(target_id)
        
        if not dmg_comp or not target_tag then return end

        local hit = false
        if dmg_comp.faction == 0 and target_tag == "Enemy" then
            hit = true
        elseif dmg_comp.faction == 1 and target_tag == "Player" then
            hit = true
        end

        if hit then
            local hp = registry:get_health(target_id)
            if hp and hp:is_alive() then
                hp:take_damage(dmg_comp.damage)
                
                if hp.current_health <= 0 and dmg_comp.faction == 0 then
                    local score_value = registry:get_score_value(target_id)
                    if score_value and not score_value.claimed then
                        score_value.claimed = true
                        local target_pos = registry:get_position(target_id)
                        if target_pos then
                            AwardScoreToNearestPlayer(registry, target_pos.position, score_value.points)
                        end
                    end
                end
                
                HandleDeath(target_id, hp)
            end
            registry:kill_entity(proj_id)
        end
    end

    if (tag1 == "Player" and tag2 == "Powerup") then
        HandlePowerupCollection(e1, e2)
        return
    elseif (tag2 == "Player" and tag1 == "Powerup") then
        HandlePowerupCollection(e2, e1)
        return
    end

    if (tag1 == "Player" and tag2 == "Enemy") or (tag1 == "Enemy" and tag2 == "Player") then
        HandleCrash(e1, e2)
        HandleCrash(e2, e1)
        return
    end

    local dmg1 = registry:get_damageable(e1)
    local dmg2 = registry:get_damageable(e2)

    if dmg1 and not dmg2 then
        HandleProjectile(e1, e2)
    elseif not dmg1 and dmg2 then
        HandleProjectile(e2, e1)
    end
end
