add_rules("mode.debug", "mode.release")

add_requires("nlohmann_json")
add_requires("raylib")

target("client")
set_kind("binary")
add_files("src/**.cpp")
add_files("../third_party/raylib-media/src/rmedia.c")
add_includedirs("src", "../third_party/raylib-media/src")
add_deps("protocol", "game_logic", "engine")
add_packages("nlohmann_json", "raylib")
add_syslinks("avcodec", "avformat", "avutil", "swresample", "swscale")
set_rundir("$(projectdir)")
