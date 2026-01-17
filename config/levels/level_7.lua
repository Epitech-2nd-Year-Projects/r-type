local Level = {
    info = { name = "Level 7", id = 7 },
    run = function()
        Wait(2.0)
        log_info("Level 7 Start - The Swarm")


        for i = 1, 10 do
            Spawn("Scout", 1700.0, 0.0, true, (i % 5 == 0))
            if i % 4 == 0 then
                 Spawn("Interceptor", 1700.0, 0.0, true, false)
            end
            Wait(0.8)
        end

        Wait(2.0)


        Spawn("Interceptor", 1700.0, 200.0, false, false)
        Spawn("Interceptor", 1700.0, 250.0, false, true)
        Spawn("Interceptor", 1700.0, 300.0, false, false)

        Wait(2.0)
        Spawn("Interceptor", 1700.0, 500.0, false, false)
        Spawn("Interceptor", 1700.0, 550.0, false, false)
        Spawn("Interceptor", 1700.0, 600.0, false, false)

        Wait(3.0)


        Spawn("Bomber", 1700.0, 100.0, false, false)
        Spawn("Bomber", 1700.0, 300.0, false, false)
        Spawn("Bomber", 1700.0, 500.0, false, false)
        Spawn("Bomber", 1700.0, 700.0, false, true)

        Wait(1.0)
        spawned = Spawn("Tank", 1700.0, 400.0, false, false)

        Wait(5.0)
        log_info("Level 7 Finished")
    end
}
return Level
