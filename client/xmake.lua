add_rules("mode.debug", "mode.release")

target("client")
set_kind("binary")
add_files("src/*.cpp")
add_deps("protocol", "game_logic", "engine")

