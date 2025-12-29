function OnCollision(e1, e2)
    local tag1 = registry:get_tag(e1)
    local tag2 = registry:get_tag(e2)

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
                      Spawn(registry, "HealthPotion", pos_comp.position.x, pos_comp.position.y)
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
