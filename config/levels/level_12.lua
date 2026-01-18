local Level = {
    info = { name = "Level 12", id = 12 },
    run = function()
        Wait(2.0)
        log_info("Level 12 Start - Final Confrontation")


        Spawn("Tank", 1700.0, 200.0, false, false)
        Spawn("Tank", 1700.0, 600.0, false, false)
        Spawn("Bomber", 1700.0, 400.0, false, false)
        Wait(1.0)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Spawn("Interceptor", 1700.0, 0.0, true, false)

        Wait(5.0)


        Spawn("Tank", 1700.0, 100.0, false, false)
        Spawn("Tank", 1700.0, 300.0, false, false)
        Spawn("Tank", 1700.0, 500.0, false, false)
        Spawn("Tank", 1700.0, 700.0, false, true)

        Wait(2.0)
        Spawn("Scout", 1700.0, 200.0, false, false)
        Spawn("Scout", 1700.0, 400.0, false, true)
        Spawn("Scout", 1700.0, 600.0, false, false)

        Wait(8.0)


        log_info("Level 12 - FINAL BOSSES!")
        Spawn("Dobkeratops", 1500.0, 150.0, false, true)
        Wait(1.0)
        Spawn("Dobkeratops", 1500.0, 600.0, false, false)


        Wait(3.0)
        Spawn("Tank", 1700.0, 100.0, false, false)
        Spawn("Tank", 1700.0, 700.0, false, false)

        Wait(5.0)
        Spawn("Interceptor", 1700.0, 0.0, true, false)
        Spawn("Interceptor", 1700.0, 0.0, true, true)
        Spawn("Interceptor", 1700.0, 0.0, true, false)

        Wait(5.0)
        log_info("Level 12 Finished - Congratulations!")
    end
}
return Level
