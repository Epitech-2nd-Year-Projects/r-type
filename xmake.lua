set_project("r-type")
set_version("0.1.0")
set_xmakever("3.0.6")

add_rules("mode.debug", "mode.release")
set_languages("cxx23")
set_warnings("all")
set_policy("package.requires_lock", false)

package("ffmpeg-prebuilt")
    set_homepage("https://github.com/BtbN/FFmpeg-Builds")
    set_description("Prebuilt FFmpeg binaries (LGPL)")
    set_license("LGPL-2.1")
    set_urls("https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n$(version)-latest-win64-lgpl-shared-$(version).zip")
    add_versions("7.1", "febe5d4d2bb2ab8fcbed15b522f2ec9768fa95316370a2f0ef2480928f72da34")
    on_load("windows", function (package)
        package:add("links", "avcodec", "avformat", "avutil", "swresample", "swscale")
        package:add("includedirs", "include")
        package:add("linkdirs", "lib")
        package:add("bindirs", "bin")
    end)
    on_install("windows", function (package)
        local version = package:version_str()
        local root = "ffmpeg-n" .. version .. "-latest-win64-lgpl-shared-" .. version
        if os.isdir(root) then
            os.cd(root)
        end
        os.cp("include", package:installdir())
        os.cp("lib", package:installdir())
        os.cp("bin", package:installdir())
    end)
package_end()

add_requires("lz4")

if is_plat("windows") then
	add_defines("NODRAWTEXT")
end

includes("engine", "server", "client", "protocol", "game_logic", "tests")

target("r-type")
set_kind("phony")
set_default(false)
add_deps("engine", "server", "client", "protocol", "game_logic")

if os.getenv("BUILD_BENCHMARKS") then
    includes("benchmarks/engine")
end

if os.getenv("BUILD_RIFT") then
    includes("rift")
end
