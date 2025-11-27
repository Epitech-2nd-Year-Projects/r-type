add_rules("mode.debug", "mode.release")

target("game_logic")
    set_kind("static")
    add_files("src/*.cpp")
    add_includedirs("include")
    add_deps("engine")
