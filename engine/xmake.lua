add_rules("mode.debug", "mode.release")

target("engine")
set_kind("static")
add_files("src/**/*.cpp")
add_includedirs("include", {public = true})
add_headerfiles("include/engine/(*.h|*.hpp)")
