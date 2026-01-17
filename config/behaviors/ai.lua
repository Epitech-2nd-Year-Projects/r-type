AIBehaviors = {}

local function GetSpeedMultiplier()
    if DifficultyModifiers and DifficultyModifiers.enemy_speed_multiplier then
        return DifficultyModifiers.enemy_speed_multiplier
    end
    return 1.0
end

function AIBehaviors.Straight(entity, dt, ai, vel, pos)
  local speed_mult = GetSpeedMultiplier()
  vel.velocity.x = -ai.speed * speed_mult
  vel.velocity.y = 0
end

function AIBehaviors.WavePattern(entity, dt, ai, vel, pos)
  local speed_mult = GetSpeedMultiplier()
  vel.velocity.x = -ai.speed * speed_mult
  ai.state_timer = ai.state_timer + dt
  vel.velocity.y = math.sin(ai.state_timer * ai.wave_frequency) * ai.wave_amplitude * speed_mult
end

function AIBehaviors.ChasePlayer(entity, dt, ai, vel, pos)
  local speed_mult = GetSpeedMultiplier()
  local found, target_x, target_y = GetNearestPlayerPosition(registry, pos.position)
  
  if found then
    local dx = target_x - pos.position.x
    local dy = target_y - pos.position.y
    local dist = math.sqrt(dx*dx + dy*dy)
    
    if dist > 0 then
      vel.velocity.x = (dx / dist) * ai.speed * speed_mult
      vel.velocity.y = (dy / dist) * ai.speed * speed_mult
    else
      vel.velocity.x = 0
      vel.velocity.y = 0
    end
  else
    vel.velocity.x = -ai.speed * speed_mult
    vel.velocity.y = 0
  end
end


function AIBehaviors.Patrol(entity, dt, ai, vel, pos)
    local speed_mult = GetSpeedMultiplier()
    vel.velocity.x = -ai.speed * 0.5 * speed_mult

    if ai.state_timer == 0 then ai.state_timer = 1 end 

    vel.velocity.y = ai.speed * ai.state_timer * speed_mult

    if pos.position.y >= ai.patrol_max.y then
        ai.state_timer = -1
        pos.position.y = ai.patrol_max.y
    elseif pos.position.y <= ai.patrol_min.y then
        ai.state_timer = 1
        pos.position.y = ai.patrol_min.y
    end
end

local DobkeratopsStates = {}

function AIBehaviors.Dobkeratops(entity, dt, ai, vel, pos)
    local speed_mult = GetSpeedMultiplier()
    local STOP_X = 600
    local ENTRY_SPEED = 100 * speed_mult
    local OSCILLATE_SPEED = 3.0
    local OSCILLATE_AMP = 80 * speed_mult
    local OSCILLATE_PERIOD = 10.0
    local OSCILLATE_DURATION = 3.0

    if not DobkeratopsStates[entity] then
        DobkeratopsStates[entity] = { 
            state = "entry",
            initial_y = pos.position.y,
            battle_timer = 0,
            oscillate_phase = 0
        }
    end
    local data = DobkeratopsStates[entity]

    if data.state == "entry" then
        if pos.position.x > STOP_X then
             vel.velocity.x = -ENTRY_SPEED
        else
             vel.velocity.x = 0
             pos.position.x = STOP_X
             data.state = "battle"
             data.initial_y = pos.position.y
             data.battle_timer = 0
        end
        vel.velocity.y = 0
    
    elseif data.state == "battle" then
        vel.velocity.x = 0
        data.battle_timer = data.battle_timer + dt
        
        local cycle_time = data.battle_timer % OSCILLATE_PERIOD
        if cycle_time < OSCILLATE_DURATION then
            data.oscillate_phase = data.oscillate_phase + dt
            vel.velocity.y = math.sin(data.oscillate_phase * OSCILLATE_SPEED) * OSCILLATE_AMP
        else
            vel.velocity.y = 0
        end
    end
end
