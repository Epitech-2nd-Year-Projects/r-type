add_rules("mode.debug", "mode.release")

target("shared")
    set_kind("static")
    set_default(false)
    add_files("src/*.cpp")