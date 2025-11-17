add_rules("mode.debug", "mode.release")

target("tools")
    set_kind("binary")
    set_default(false)
    add_files("src/*.cpp")
