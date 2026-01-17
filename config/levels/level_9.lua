local Level = {
    info = { name = "Level 9", id = 9 },
    run = function()
        Wait(2.0)
        log_info("Level 9 Start - Danger Zone")


        Spawn("Tank", 1700.0, 400.0, false, false)
        Spawn("Interceptor", 1700.0, 200.0, false, false)
        Spawn("Interceptor", 1700.0, 600.0, false, false)
        Spawn("Scout", 1750.0, 300.0, false, false)
        Spawn("Scout", 1750.0, 500.0, false, false)

        Wait(4.0)

        Spawn("Tank", 1700.0, 300.0, false, true)
        Spawn("Tank", 1700.0, 500.0, false, false)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Spawn("Interceptor", 1700.0, 0.0, true, false)

        Wait(6.0)


        for i = 1, 8 do
            Spawn("Interceptor", 1700.0 + (i * 50), 0.0, true, false)
        end

        Wait(4.0)


        log_info("Level 9 - Boss Fight!")
        Spawn("Dobkeratops", 1500.0, 300.0, false, true)


        Wait(4.0)
        Spawn("Bomber", 1700.0, 100.0, false, false)
        Spawn("Bomber", 1700.0, 700.0, false, false)

        Wait(4.0)
        Spawn("Bomber", 1700.0, 200.0, false, false)
        Spawn("Bomber", 1700.0, 600.0, false, true)

        Wait(4.0)
        Spawn("Bomber", 1700.0, 300.0, false, false)
        Spawn("Bomber", 1700.0, 500.0, false, false)

        Wait(5.0)
        log_info("Level 9 Finished")
    end
}
return Level
