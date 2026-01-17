add_requires("raylib", "imgui")

target("rift_client")
    set_kind("binary")
    set_default(false)
    add_files("src/**.cpp")
    add_includedirs("src")
    add_deps("protocol", "rift_game_logic", "engine", "engine_debug")
    add_packages("raylib", "imgui")
    set_rundir("$(projectdir)")
    after_load(function (target)
        target:add("syslinks", "engine_debug", "imgui")
        target:add("linkdirs", "$(builddir)/$(plat)/$(arch)/$(mode)")
        local imgui = target:pkg("imgui")
        if imgui then
            target:add("linkdirs", imgui:installdir("lib"))
        end
    end)
