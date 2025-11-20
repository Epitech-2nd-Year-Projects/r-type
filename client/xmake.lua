set_policy("package.require_confirm", true)

add_rules("mode.debug", "mode.release")

add_requires("raylib", {system = false})

target("client")
    set_kind("binary")
    set_languages("cxx17")

    add_includedirs("include", {public = true})
    add_files("main.cpp", "src/**.cpp")
    add_packages("raylib")
    add_deps("protocol", "game_logic", "engine")