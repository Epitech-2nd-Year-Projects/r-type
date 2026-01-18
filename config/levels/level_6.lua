local Level = {
    info = { name = "Level 6", id = 6 },
    run = function()
        Wait(2.0)
        log_info("Level 6 Start - Boss Approach")


        for i = 1, 5 do
            Spawn("Bomber", 1700.0, 100 + (i * 100), false, false)
            if i % 2 == 0 then
                Spawn("Scout", 1700.0, 50 + (i * 100), false, false)
            end
            Wait(0.5)
        end

        Wait(3.0)


        Spawn("Tank", 1700.0, 200.0, false, false)
        Spawn("Tank", 1700.0, 400.0, false, true)
        Spawn("Tank", 1700.0, 600.0, false, false)

        Wait(6.0)


        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Wait(1.0)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Spawn("Interceptor", 1700.0, 0.0, true, false)

        Wait(5.0)


        log_info("Level 6 - Boss Fight!")
        Spawn("Dobkeratops", 1500.0, 300.0, false, true)


        Wait(5.0)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Wait(5.0)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Wait(5.0)
        Spawn("Interceptor", 1700.0, 0.0, true, true)

        Wait(5.0)
        log_info("Level 6 Finished")
    end
}
return Level
