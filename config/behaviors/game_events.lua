-- Game Events Logic
-- Handles collisions and entity death (health, score, drops)

GameEvents = {}

-- Constants
local CRASH_DAMAGE = 100

-- Helper to check tags
local function HasTag(entity, tag)
    local t = GetTag(registry, entity)
    return t and t.tag == tag
end

function GameEvents.HandleCollision(e1, e2)
    -- 1. Check for Crash (Player vs Enemy)
    local tag1 = GetTag(registry, e1)
    local tag2 = GetTag(registry, e2)

    if tag1 and tag2 then
        local p1 = (tag1.tag == "Player")
        local e1_enemy = (tag1.tag == "Enemy")
        local p2 = (tag2.tag == "Player")
        local e2_enemy = (tag2.tag == "Enemy")

        if (p1 and e2_enemy) or (e1_enemy and p2) then
            -- Crash Logic
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
            return -- Crash resolved
        end
    end

    -- 2. Check for Projectile Hits
    -- Helper to resolve projectile (proj) vs target (targ)
    local function ResolveProjectile(proj, targ)
        local dmg = GetDamageable(registry, proj)
        if not dmg then return false end

        local target_tag = GetTag(registry, targ)
        if not target_tag then return false end
        
        local hit = false
        -- Faction 0 (Player) hits Enemy
        if dmg.faction == 0 and target_tag.tag == "Enemy" then
            hit = true
        end
        -- Faction 1 (Enemy) hits Player
        if dmg.faction == 1 and target_tag.tag == "Player" then
            hit = true
        end

        if hit then
            local hp = GetHealth(registry, targ)
            if hp then
                hp:take_damage(dmg.damage)
                hp.last_attacker_id = dmg.owner_id
            end
            -- Kill projectile
            registry:kill_entity(proj)
            return true
        end
        return false
    end

    -- Try resolving e1 as projectile hitting e2
    if ResolveProjectile(e1, e2) then return end
    -- Try resolving e2 as projectile hitting e1
    ResolveProjectile(e2, e1)
end

function GameEvents.HandleDeath(entity)
    -- 1. Player Death Logic
    local player = GetPlayer(registry, entity)
    if player then
        if player.lives > 0 then
            player.lives = player.lives - 1
            print("[Lua] Player " .. player.player_id .. " died. Lives: " .. player.lives .. ". Respawning.")
            if NotifyPlayerDeath then
                 NotifyPlayerDeath(player.player_id, player.lives)
            end
            
            -- Respawn Logic
            local hp = GetHealth(registry, entity)
            if hp then hp.current_health = hp.max_health end
            
            -- Reset Position (Simplified for now, ideal to expose Respawn helper)
            local pos = GetPosition(registry, entity)
            if pos then
                pos.x = 100 + (player.player_slot * 50) -- Approximate offset
                pos.y = 540 -- kRespawnY
            end
            return -- Do not kill entity
        else
            print("[Lua] Player " .. player.player_id .. " Game Over.")
            -- Permadeath: Kill entity
            registry:kill_entity(entity)
            return
        end
    end

    -- 2. Score Logic
    local score_val = GetScoreValue(registry, entity)
    local hp = GetHealth(registry, entity)
    
    if score_val and not score_val.claimed and hp and hp.last_attacker_id then
        local attacker = GetPlayer(registry, hp.last_attacker_id)
        if attacker then
            attacker.score = attacker.score + score_val.points
            score_val.claimed = true
        end
    end

    -- 3. Powerup Drop Logic
    local drops = GetDropsPowerup(registry, entity)
    if drops then
        -- Spawn HealthPotion at entity position
        local pos = GetPosition(registry, entity)
        if pos then
            Spawn(registry, "HealthPotion", pos.x, pos.y)
        end
    end
    
    -- Finally kill the entity
    registry:kill_entity(entity)
end
