AIBehaviors = {}

function AIBehaviors.Straight(entity, dt, ai, vel, pos)
  vel.velocity.x = -ai.speed
  vel.velocity.y = 0
end

function AIBehaviors.WavePattern(entity, dt, ai, vel, pos)
  vel.velocity.x = -ai.speed
  ai.state_timer = ai.state_timer + dt
  vel.velocity.y = math.sin(ai.state_timer * ai.wave_frequency) * ai.wave_amplitude
end

function AIBehaviors.ChasePlayer(entity, dt, ai, vel, pos)
  local found, target_x, target_y = GetNearestPlayerPosition(registry, pos.position)
  
  if found then
    local dx = target_x - pos.position.x
    local dy = target_y - pos.position.y
    local dist = math.sqrt(dx*dx + dy*dy)
    
    if dist > 0 then
      vel.velocity.x = (dx / dist) * ai.speed
      vel.velocity.y = (dy / dist) * ai.speed
    else
      vel.velocity.x = 0
      vel.velocity.y = 0
    end
  else
    vel.velocity.x = -ai.speed
    vel.velocity.y = 0
  end
end

function AIBehaviors.Patrol(entity, dt, ai, vel, pos)
    vel.velocity.x = -ai.speed * 0.5 

    if ai.state_timer == 0 then ai.state_timer = 1 end 

    vel.velocity.y = ai.speed * ai.state_timer

    if pos.position.y >= ai.patrol_max.y then
        ai.state_timer = -1
        pos.position.y = ai.patrol_max.y
    elseif pos.position.y <= ai.patrol_min.y then
        ai.state_timer = 1
        pos.position.y = ai.patrol_min.y
    end
end
