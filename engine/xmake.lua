add_rules("mode.debug", "mode.release")

add_requires("asio", "raylib", "lua", "sol2", "imgui")

target("rlImGui")
    set_kind("static")
    add_files("../third_party/rlImGui/rlImGui.cpp")
    add_headerfiles("../third_party/rlImGui/rlImGui.h")
    add_includedirs("../third_party/rlImGui", { public = true })
    add_defines("NO_FONT_AWESOME", { public = true })
    add_packages("imgui", "raylib", { public = true })

function engine_module(name, deps, packages)
    target("engine_" .. name)
        set_kind("static")
        add_files("src/" .. name .. "/**.cpp")
        add_headerfiles("include/engine/" .. name .. "/**.h")
        add_includedirs("include", { public = true })
        if deps then
            add_deps(deps)
        end
        if packages then
            add_packages(packages, { public = true })
        end
end

engine_module("core", {})
engine_module("util", {"engine_core"})
engine_module("math", {"engine_core"})
engine_module("time", {"engine_core"})
engine_module("event", {"engine_core"})
engine_module("resource", {"engine_core"})
engine_module("ecs", {"engine_core", "engine_util", "engine_time"})
engine_module("input", {"engine_event", "engine_math"})

engine_module("net", {"engine_event", "engine_util", "engine_core"}, "asio")
engine_module("audio", {"engine_resource"}, "raylib")
engine_module("render", {"engine_math", "engine_resource"}, "raylib")
engine_module("ui", {"engine_render", "engine_input"})
engine_module("profiling", {"engine_render", "engine_time", "engine_math"})
engine_module("scripting", {"engine_core", "engine_util", "engine_ecs", "engine_math", "engine_render", "engine_event", "engine_input", "engine_audio"}, {"lua", "sol2"})
engine_module("console", {"engine_core", "engine_util", "engine_render", "engine_input", "engine_time", "engine_scripting"}, {"lua", "sol2"})
engine_module("app", {"engine_core", "engine_audio", "engine_render", "engine_input", "engine_time", "engine_ui", "engine_scripting", "engine_console"})

target("engine_runtime")
    set_kind("static")
    add_files("src/game_runtime.cpp")
    add_headerfiles("include/engine/game_runtime.h")
    add_includedirs("include", { public = true })
    add_deps("engine_core", "engine_ecs", "engine_event", "engine_time", "engine_net", "engine_render", "engine_audio")

target("engine_debug")
    set_kind("static")
    add_files("src/debug/**.cpp")
    add_headerfiles("include/engine/debug/**.h")
    add_includedirs("include", { public = true })
    add_deps("engine_core", "engine_ecs", "engine_input", "engine_net", "rlImGui")
    add_packages("imgui", "raylib", { public = true })

target("engine")
    set_kind("phony")
    add_deps("engine_core", "engine_util", "engine_math", "engine_time", "engine_event", "engine_resource", "engine_ecs", "engine_input", "engine_net", "engine_audio", "engine_render", "engine_ui", "engine_profiling", "engine_scripting", "engine_console", "engine_app", "engine_runtime", "engine_debug")
    add_headerfiles("include/engine/*.h")
