add_rules("mode.debug", "mode.release")

add_requires("asio", "raylib")

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
engine_module("profiler", {"engine_core", "engine_time", "engine_render", "engine_input"})
engine_module("app", {"engine_core", "engine_audio", "engine_render", "engine_input", "engine_time", "engine_ui", "engine_profiler"})

target("engine")
    set_kind("phony")
    add_deps("engine_core", "engine_util", "engine_math", "engine_time", "engine_event", "engine_resource", "engine_ecs", "engine_input", "engine_net", "engine_audio", "engine_render", "engine_ui", "engine_app")
    add_headerfiles("include/engine/*.h")
