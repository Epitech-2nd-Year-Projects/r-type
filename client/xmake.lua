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
if is_plat("windows") then
    local ffmpeg_dir = os.getenv("FFMPEG_DIR")
    if ffmpeg_dir then
        add_includedirs(path.join(ffmpeg_dir, "include"))
        add_linkdirs(path.join(ffmpeg_dir, "lib"))
    end
    add_links("avcodec", "avformat", "avutil", "swresample", "swscale")
else
    add_syslinks("avcodec", "avformat", "avutil", "swresample", "swscale")
end
set_rundir("$(projectdir)")
