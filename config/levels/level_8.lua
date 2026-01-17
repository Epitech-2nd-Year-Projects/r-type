local Level = {
    info = { name = "Level 8", id = 8 },
    run = function()
        Wait(2.0)
        log_info("Level 8 Start - Heavy Armor")


        Spawn("Tank", 1700.0, 200.0, false, true)
        Wait(0.5)
        Spawn("Tank", 1700.0, 600.0, false, false)
        Wait(2.0)

        Spawn("Tank", 1700.0, 300.0, false, false)
        Wait(0.5)
        Spawn("Scout", 1700.0, 100.0, true, false)
        Spawn("Scout", 1700.0, 700.0, true, false)
        Spawn("Tank", 1700.0, 500.0, false, true)

        Wait(4.0)


        for i = 1, 6 do
            Spawn("Bomber", 1700.0, 100 + (i * 100), false, false)
        end

        Wait(2.0)
        Spawn("Tank", 1700.0, 400.0, false, false)

        Wait(5.0)


        Spawn("Interceptor", 1800.0, 100.0, true, false)
        Spawn("Interceptor", 1800.0, 700.0, true, false)
        Spawn("Tank", 1700.0, 150.0, false, false)
        Spawn("Tank", 1700.0, 350.0, false, true)
        Spawn("Tank", 1700.0, 550.0, false, false)
        Spawn("Interceptor", 1800.0, 0.0, true, true)
        Spawn("Interceptor", 1800.0, 0.0, true, false)

        Wait(8.0)
        log_info("Level 8 Finished")
    end
}
return Level
