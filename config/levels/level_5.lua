local Level = {
    info = { name = "Level 5", id = 5 },
    run = function()
        Wait(2.0)
        log_info("Level 5 Start - Interceptor Swarm")


        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Wait(0.5)
        Spawn("Interceptor", 1700.0, 0.0, true, true)
        Wait(0.5)
        Spawn("Interceptor", 1700.0, 0.0, true, false)

        Wait(3.0)


        Spawn("Bomber", 1700.0, 200.0, false, false)
        Spawn("Bomber", 1700.0, 600.0, false, false)
        Wait(1.0)
        Spawn("Interceptor", 1700.0, 400.0, true, true)
        Spawn("Interceptor", 1700.0, 400.0, true, false)

        Wait(4.0)


        Spawn("Scout", 1700.0, 100.0, true, false)
        Spawn("Scout", 1700.0, 700.0, true, false)
        Spawn("Interceptor", 1700.0, 300.0, false, true)
        Spawn("Interceptor", 1700.0, 500.0, false, false)

        Wait(3.0)
        Spawn("Tank", 1700.0, 400.0, false, true)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Spawn("Interceptor", 1700.0, 0.0, true, false)

        Wait(5.0)
        log_info("Level 5 Finished")
    end
}
return Level
