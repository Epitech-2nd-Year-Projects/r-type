add_rules("mode.debug", "mode.release")

add_requires("nlohmann_json", "raylib")
add_requires("ffmpeg-prebuilt 7.1", {alias = "ffmpeg"})

 target("client")
 set_policy("check.target_package_licenses", false)
 set_kind("binary")
 add_files("src/**.cpp")
 add_files("../third_party/raylib-media/src/rmedia.c")
 add_includedirs("src", "../third_party/raylib-media/src")
 add_deps("protocol", "game_logic", "engine", "engine_debug")
 add_packages("nlohmann_json", "raylib", "ffmpeg")
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
 set_rundir("$(projectdir)")

