local Level = {
    info = { name = "Level 3", id = 3 },
    run = function()
        Wait(2.0)

        Spawn("Bomber", 1700.0, 100.0, true, false)
        Wait(1.0)
        Spawn("Bomber", 1700.0, 200.0, true, false)
        Wait(1.0)
        Spawn("Bomber", 1700.0, 300.0, true, false)
        Wait(1.0)
        Spawn("Bomber", 1700.0, 400.0, true, false)
        Wait(1.0)
        Spawn("Bomber", 1700.0, 500.0, true, false)
        Wait(1.0)
        Spawn("Bomber", 1700.0, 150.0, true, false)
        Wait(1.0)
        Spawn("Bomber", 1700.0, 250.0, true, false)
        Wait(1.0)
        Spawn("Bomber", 1700.0, 350.0, true, false)
        Wait(1.0)
        Spawn("Bomber", 1700.0, 450.0, true, false)
        Wait(1.0)
        Spawn("Bomber", 1700.0, 550.0, true, false)
        Wait(8.0)
        Spawn("Dobkeratops", 1500.0, 300.0, false, false)
        Wait(3.0)
        Spawn("Scout", 1700.0, 50.0, false, false)
        Spawn("Scout", 1700.0, 150.0, false, false)
        Spawn("Scout", 1700.0, 250.0, false, false)
        Spawn("Scout", 1700.0, 650.0, false, false)
        Spawn("Scout", 1700.0, 750.0, false, false)
        Spawn("Scout", 1700.0, 850.0, false, false)


        
        log_info("Level 3 - Boss Fight!")
    end
}
return Level
