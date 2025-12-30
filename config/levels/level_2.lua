local Level = {
    info = { name = "Level 2", id = 2 },
    run = function()
        Wait(2.0)
        Spawn("Scout", 1650.0, 0.0, true, false)
        
        Wait(0.5)
        Spawn("Scout", 1650.0, 0.0, true, false)
        
        Wait(0.5)
        Spawn("Scout", 1650.0, 0.0, true, false)
        
        Wait(2.0)
        Spawn("Bomber", 1700.0, 800.0, true, false)
        
        Wait(2.0)
        Spawn("Bomber", 1700.0, 0.0, true, false)
        Spawn("Bomber", 1700.0, 0.0, true, false)
        
        Wait(3.0)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        
        Wait(0.2)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        
        Wait(0.2)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        
        Wait(1.6)
        Spawn("Scout", 1700.0, 100.0, true, false)
        Spawn("Interceptor", 1700.0, 800.0, true, false)
        
        Wait(3.0)
        Spawn("Scout", 1650.0, 0.0, true, true)
        
        Wait(3.0)
        Spawn("Scout", 1700.0, 100.0, true, false)
        Spawn("Bomber", 1700.0, 700.0, true, false)
        
        Wait(4.0)
        Spawn("Interceptor", 1700.0, 400.0, true, false)
        
        Wait(0.2)
        Spawn("Scout", 1700.0, 200.0, true, false)
        Spawn("Scout", 1700.0, 600.0, true, false)
        
        Wait(3.8)
        Spawn("Scout", 1650.0, 0.0, true, false)
        Spawn("Bomber", 1700.0, 0.0, true, false)
        Spawn("Interceptor", 1750.0, 0.0, true, false)
        
        Wait(4.0)
        Spawn("Bomber", 1700.0, 100.0, true, true)
        Spawn("Bomber", 1700.0, 700.0, true, false)
        
        Wait(0.5)
        Spawn("Interceptor", 1700.0, 400.0, true, false)
        
        print("Level 2 Finished")
    end
}
return Level
