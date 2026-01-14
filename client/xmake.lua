add_rules("mode.debug", "mode.release")

add_requires("nlohmann_json")
if is_plat("windows") then
    add_requires("raylib")
else
    add_requires("raylib", "ffmpeg")
end

 target("client")
 set_policy("check.target_package_licenses", false)
 set_kind("binary")
 add_files("src/**.cpp")
 add_files("../third_party/raylib-media/src/rmedia.c")
 add_includedirs("src", "../third_party/raylib-media/src")
 add_deps("protocol", "game_logic", "engine", "engine_debug")
 add_packages("nlohmann_json", "raylib")
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
 set_rundir("$(projectdir)")

