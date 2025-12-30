local Level = {
    info = { name = "Level 1", id = 1 },
    run = function()
        Wait(2.0)
        Spawn("Scout", 1650.0, 0.0, true, false)
        
        Wait(1.0)
        Spawn("Scout", 1650.0, 0.0, true, true)
        
        Wait(1.0)
        Spawn("Scout", 1650.0, 0.0, true, false)
        
        Wait(4.0)
        Spawn("Bomber", 1700.0, 0.0, true, false)
        
        Wait(0.5)
        Spawn("Bomber", 1700.0, 0.0, true, false)
        
        Wait(0.5)
        Spawn("Bomber", 1700.0, 0.0, true, false)
        
        Wait(3.0)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        
        Wait(0.5)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        
        Wait(0.5)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        
        Wait(2.0)
        Spawn("Scout", 1700.0, 0.0, true, false)
        Spawn("Scout", 1700.0, 0.0, true, false)
        
        Wait(1.0)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        
        Wait(4.0)
        Spawn("Bomber", 1700.0, 0.0, true, false)
        Spawn("Bomber", 1700.0, 0.0, true, false)
        
        Wait(2.0)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        
        Wait(2.0)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        
        print("Level 1 Finished")
    end
}
return Level
