LevelManager = LevelManager or {}
LevelManager.current_co = nil
LevelManager.current_level = nil
LevelManager.config_path = ""
LevelManager.current_id = 1
LevelManager.state = "IDLE"
LevelManager.transition_timer = 0
LevelManager.TRANSITION_DELAY = 3.0

function LevelManager.Init(config_path)
    LevelManager.config_path = config_path .. "/levels/"
    print("LevelManager initialized with path: " .. LevelManager.config_path)
end

function LevelManager.Update(dt, enemies_alive)
    if LevelManager.current_co and coroutine.status(LevelManager.current_co) ~= "dead" then
        LevelManager.state = "RUNNING"
        local success, err = coroutine.resume(LevelManager.current_co, dt)
        if not success then
            print("Error in Level Coroutine: " .. err)
        end
    elseif LevelManager.current_co and coroutine.status(LevelManager.current_co) == "dead" then
        if LevelManager.state == "RUNNING" then
            LevelManager.state = "WAITING_CLEAR"
            print("Level Script Finished. Waiting for enemies to clear...")
        end
    end

    if LevelManager.state == "WAITING_CLEAR" then
        if not enemies_alive then
            LevelManager.state = "TRANSITION"
            LevelManager.transition_timer = 0
            print("Area clear. Transitioning in " .. LevelManager.TRANSITION_DELAY .. "s")
        end
    elseif LevelManager.state == "TRANSITION" then
        LevelManager.transition_timer = LevelManager.transition_timer + dt
        if LevelManager.transition_timer >= LevelManager.TRANSITION_DELAY then
            LevelManager.LoadNextLevel()
        end
    end
end

function LevelManager.LoadNextLevel()
    local next_id = LevelManager.current_id + 1
    local success = LevelManager.LoadLevel(next_id)
    if not success then
        print("Next level not found (" .. next_id .. "), looping back to Level 1")
        LevelManager.LoadLevel(1)
    end
end

function LevelManager.LoadLevel(id)
    local path = LevelManager.config_path .. "level_" .. id .. ".lua"
    
    local level_env = setmetatable({}, { __index = _G })
    
    level_env.Wait = function(seconds)
        local elapsed = 0
        while elapsed < seconds do
            local dt = coroutine.yield()
            elapsed = elapsed + (dt or 0)
        end
    end
    
    level_env.Spawn = function(type_name, x, y, random_y, drops_powerup)
        local final_y = y
        if random_y then
             final_y = math.random(50, 700)
        end
        local entity = Spawn(registry, type_name, x, final_y)
        if entity and drops_powerup then
             if AddDropsPowerup then AddDropsPowerup(registry, entity) end
        end
        return entity
    end

    level_env.SpawnBoss = function(type_name)
         Spawn(registry, type_name, 1200, 400)
    end
    
    local level_chunk, err = loadfile(path, "t", level_env)
    
    if not level_chunk then
        print("Failed to load level: " .. path .. " Error: " .. (err or "unknown"))
        return false
    end
    
    local level_table = level_chunk()
    
    LevelManager.current_level = level_table
    LevelManager.current_id = id
    
    if level_table.run then
        LevelManager.current_co = coroutine.create(function(dt)
             level_table.run()
        end)
    end
    
    LevelManager.state = "RUNNING"
    print("Loaded Level " .. id .. ": " .. (level_table.info.name or "Unknown"))
    return true
end
