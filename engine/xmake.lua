add_rules("mode.debug", "mode.release")

add_requires("asio", "raylib")

target("engine")
set_kind("static")
add_files("src/**/*.cpp")
add_includedirs("include", { public = true })
add_headerfiles("include/engine/(**.h|**.hpp)")
add_packages("asio", "raylib")
