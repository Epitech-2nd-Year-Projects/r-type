add_rules("mode.debug", "mode.release")

target("game_logic")
set_kind("static")
set_default(false)
add_files("src/*.cpp")
add_deps("engine")
