local Level = {
    info = { name = "Level 4", id = 4 },
    run = function()
        Wait(2.0)
        log_info("Level 4 Start - Tank Introduction")


        Spawn("Tank", 1700.0, 200.0, false, false)
        Wait(1.0)
        Spawn("Tank", 1700.0, 600.0, false, true)

        Wait(5.0)


        Spawn("Tank", 1700.0, 400.0, false, true)
        Spawn("Scout", 1700.0, 200.0, false, false)
        Spawn("Scout", 1700.0, 600.0, false, false)

        Wait(4.0)
        Spawn("Scout", 1700.0, 300.0, false, false)
        Spawn("Interceptor", 1700.0, 100.0, true, false)
        Spawn("Scout", 1700.0, 500.0, false, false)
        Spawn("Interceptor", 1700.0, 700.0, true, false)

        Wait(4.0)


        Spawn("Tank", 1700.0, 100.0, false, false)
        Spawn("Tank", 1700.0, 300.0, false, false)
        Spawn("Tank", 1700.0, 500.0, false, true)
        Spawn("Tank", 1700.0, 700.0, false, false)

        Wait(8.0)

        spawned = Spawn("Interceptor", 1700.0, 400.0, true, true)

        log_info("Level 4 Finished")
    end
}
return Level
