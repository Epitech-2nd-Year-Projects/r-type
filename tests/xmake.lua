if is_plat("windows") then
    add_requires("gtest", "nlohmann_json", "raylib")
else
    add_requires("gtest", "nlohmann_json", "ffmpeg", "raylib")
end

 target("client_tests")
     set_kind("binary")
     set_default(false)
     add_packages("gtest", "nlohmann_json", "raylib")
     if is_plat("windows") then
         local ffmpeg_dir = os.getenv("FFMPEG_DIR")
         if not ffmpeg_dir or #ffmpeg_dir == 0 then
             raise("FFMPEG_DIR must be set on Windows to use prebuilt FFmpeg")
         end
         add_includedirs(path.join(ffmpeg_dir, "include"))
         add_linkdirs(path.join(ffmpeg_dir, "lib"))
         add_links("avcodec", "avformat", "avutil", "swresample", "swscale")
         add_runenvs("PATH", path.join(ffmpeg_dir, "bin"))
     else
         add_packages("ffmpeg")
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
