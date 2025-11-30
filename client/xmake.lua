add_rules("mode.debug", "mode.release")

target("client")
set_kind("binary")
add_files("src/*.cpp")
add_includedirs("$(projectdir)/engine/include", "$(projectdir)/protocol/include",
                {public = true})
add_deps("engine", "protocol", "game_logic")
