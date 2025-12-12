add_rules("mode.debug", "mode.release")

add_requires("nlohmann_json")

target("client")
set_kind("binary")
add_files("src/**.cpp")
add_includedirs("src")
add_deps("protocol", "game_logic", "engine")
add_packages("nlohmann_json")
set_rundir("$(projectdir)")
