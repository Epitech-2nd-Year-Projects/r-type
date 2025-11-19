add_rules("mode.debug", "mode.release")

target("engine")
set_kind("static")
add_files("src/**/*.cpp")
add_includedirs("include")
add_headerfiles("include/engine/*.hpp")
