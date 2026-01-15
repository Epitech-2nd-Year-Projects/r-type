if is_plat("windows") then
    add_requires("gtest", "nlohmann_json", "raylib")
    add_requires("ffmpeg-prebuilt 7.1", {alias = "ffmpeg"})
else
    add_requires("gtest", "nlohmann_json", "ffmpeg", "raylib")
end

 target("client_tests")
     set_kind("binary")
     set_default(false)
     add_packages("gtest", "nlohmann_json", "raylib", "ffmpeg")
     if is_plat("windows") then
         after_load(function (target)
             local ffmpeg = target:pkg("ffmpeg")
             if ffmpeg then
                 target:add("runenvs", "PATH", ffmpeg:installdir("bin"))
             end
         end)
         after_build(function (target)
             local ffmpeg = target:pkg("ffmpeg")
             if ffmpeg then
                 os.cp(path.join(ffmpeg:installdir("bin"), "*.dll"), target:targetdir())
             end
         end)
     end
     add_defines("RTYPE_TESTING")
     add_files("client/*.cpp")
     add_files("../client/src/**.cpp|main.cpp")
     add_files("../third_party/raylib-media/src/rmedia.c")
     add_includedirs("../client/src", "../third_party/raylib-media/src")
     add_deps("protocol", "engine", "game_logic")
     add_tests("client_tests")


target("engine_tests")
    set_kind("binary")
    set_default(false)
    add_packages("gtest")
    add_files("engine/*.cpp")
    add_files("engine/util/*.cpp")
    add_files("engine/math/*.cpp")
    add_files("engine/render/*.cpp")
    add_files("engine/net/*.cpp")
    add_files("engine/audio/*.cpp")
    add_links("gtest_main")
    add_deps("engine", "engine_debug", "protocol")
    add_tests("engine_tests")

target("game_logic_tests")
    set_kind("binary")
    set_default(false)
    add_files("game_logic/*.cpp")
    add_packages("gtest")
    add_deps("engine", "game_logic")
    add_files("game_logic/*.cpp")
    add_tests("game_logic_tests")

target("protocol_tests")
    set_kind("binary")
    set_default(false)
    add_files("protocol/**.cpp")
    add_packages("gtest")
    add_deps("protocol", "engine")
    add_tests("protocol_tests")

target("server_tests")
    set_kind("binary")
    set_default(false)
    add_files("server/*.cpp")
    add_files("../server/src/*.cpp|main.cpp")
    add_packages("gtest")
    add_links("gtest_main")
    add_deps("server", "engine", "protocol", "game_logic")
    add_includedirs("../server/src") 
    add_tests("server_tests")
