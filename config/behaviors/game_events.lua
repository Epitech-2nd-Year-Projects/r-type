GameEvents = {}

local CRASH_DAMAGE = 100

local function HasTag(entity, tag)
    local t = GetTag(registry, entity)
    return t and t.tag == tag
end

function GameEvents.HandleCollision(e1, e2)
    local tag1 = GetTag(registry, e1)
    local tag2 = GetTag(registry, e2)

    if tag1 and tag2 then
        local p1 = (tag1.tag == "Player")
        local e1_enemy = (tag1.tag == "Enemy")
        local p2 = (tag2.tag == "Player")
        local e2_enemy = (tag2.tag == "Enemy")

        if (p1 and e2_enemy) or (e1_enemy and p2) then
            local h1 = GetHealth(registry, e1)
            local h2 = GetHealth(registry, e2)

            if h1 then 
                h1:take_damage(CRASH_DAMAGE) 
                h1.last_attacker_id = e2
            end
            if h2 then 
                h2:take_damage(CRASH_DAMAGE) 
                h2.last_attacker_id = e1
            end
            return
        end
    end
    local function ResolveProjectile(proj, targ)
        local dmg = GetDamageable(registry, proj)
        if not dmg then return false end

        local target_tag = GetTag(registry, targ)
        if not target_tag then return false end
        
        local hit = false
        if dmg.faction == 0 and target_tag.tag == "Enemy" then
            hit = true
        end
        if dmg.faction == 1 and target_tag.tag == "Player" then
            hit = true
        end

        if hit then
            local hp = GetHealth(registry, targ)
            if hp then
                hp:take_damage(dmg.damage)
                hp.last_attacker_id = dmg.owner_id
            end
            registry:kill_entity(proj)
            return true
        end
        return false
    end

    if ResolveProjectile(e1, e2) then return end
    ResolveProjectile(e2, e1)
end

function GameEvents.HandleDeath(entity)
    local player = GetPlayer(registry, entity)
    if player then
        if player.lives > 0 then
            player.lives = player.lives - 1
            print("[Lua] Player " .. player.player_id .. " died. Lives: " .. player.lives .. ". Respawning.")
            if NotifyPlayerDeath then
                 NotifyPlayerDeath(player.player_id, player.lives)
            end
            
            local hp = GetHealth(registry, entity)
            if hp then hp.current_health = hp.max_health end
            local pos = GetPosition(registry, entity)
            if pos then
                pos.x = 100 + (player.player_slot * 50)
                pos.y = 540
            end
            return
        else
            print("[Lua] Player " .. player.player_id .. " Game Over.")
            registry:kill_entity(entity)
            return
        end
    end

    local score_val = GetScoreValue(registry, entity)
    local hp = GetHealth(registry, entity)
    
    if score_val and not score_val.claimed and hp and hp.last_attacker_id then
        local attacker = GetPlayer(registry, hp.last_attacker_id)
        if attacker then
            attacker.score = attacker.score + score_val.points
            score_val.claimed = true
        end
    end

    local drops = GetDropsPowerup(registry, entity)
    if drops then
        local pos = GetPosition(registry, entity)
        if pos then
            Spawn(registry, "HealthPotion", pos.x, pos.y)
        end
    end
    registry:kill_entity(entity)
end
