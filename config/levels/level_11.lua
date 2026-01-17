local Level = {
    info = { name = "Level 11", id = 11 },
    run = function()
        Wait(2.0)
        log_info("Level 11 Start - Elite Survival")


        Spawn("Scout", 1700.0, 200.0, false, false)
        Spawn("Scout", 1700.0, 400.0, false, true)
        Spawn("Scout", 1700.0, 600.0, false, false)
        Wait(1.5)

        Spawn("Tank", 1700.0, 100.0, false, false)
        Spawn("Tank", 1700.0, 700.0, false, false)
        Wait(1.0)
        Spawn("Tank", 1700.0, 300.0, false, false)
        Spawn("Tank", 1700.0, 500.0, false, true)

        Wait(3.0)


        for i = 1, 15 do
             Spawn("Interceptor", 1700.0, 0.0, true, (i % 5 == 0))
             Wait(0.2)
        end

        Wait(2.0)


        Spawn("Tank", 1700.0, 400.0, false, false)
        Spawn("Bomber", 1700.0, 200.0, false, false)
        Spawn("Bomber", 1700.0, 600.0, false, false)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Spawn("Interceptor", 1700.0, 0.0, true, true)

        Wait(2.0)


        for i = 1, 20 do
             Spawn("Interceptor", 1700.0, math.random(0, 800), true, false)
             Wait(0.1)
        end

        Wait(8.0)
        log_info("Level 11 Finished")
    end
}
return Level
