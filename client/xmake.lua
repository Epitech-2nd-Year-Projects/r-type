add_rules("mode.debug", "mode.release")

add_requires("nlohmann_json")
add_requires("raylib", "ffmpeg")

target("client")
set_kind("binary")
add_files("src/**.cpp")
add_files("../third_party/raylib-media/src/rmedia.c")
add_includedirs("src", "../third_party/raylib-media/src")
add_deps("protocol", "game_logic", "engine", "engine_debug")
add_packages("nlohmann_json", "raylib")
add_packages("ffmpeg")
set_rundir("$(projectdir)")
