local Level = {
    info = { name = "Level 10", id = 10 },
    run = function()
        Wait(2.0)
        log_info("Level 10 Start - Endurance")


        for i = 1, 15 do
            local ra = math.random()
            if ra < 0.3 then
                Spawn("Tank", 1700.0, 0.0, true, false)
            elseif ra < 0.6 then
                Spawn("Bomber", 1700.0, 0.0, true, false)
            else
                Spawn("Interceptor", 1700.0, 0.0, true, false)
            end

            if i % 3 == 0 then
                 Spawn("Scout", 1700.0, 0.0, true, true)
            end
            Wait(1.5)
        end

        Wait(5.0)
        log_info("Level 10 Finished")
    end
}
return Level
