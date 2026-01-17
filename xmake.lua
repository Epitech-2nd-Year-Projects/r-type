set_project("r-type")
set_version("0.1.0")
set_xmakever("3.0.6")

add_rules("mode.debug", "mode.release")
set_languages("cxx23")
set_warnings("all")
set_policy("package.requires_lock", false)

local FFMPEG_AUTOBUILD_TAG = "autobuild-2026-01-17-13-00"
local FFMPEG_BUILD_SUFFIX = "7.1.3-34-g09dc319bf3"

package("ffmpeg-prebuilt")
    set_homepage("https://github.com/BtbN/FFmpeg-Builds")
    set_description("Prebuilt FFmpeg binaries (LGPL)")
    set_license("LGPL-2.1")
    set_urls("https://github.com/BtbN/FFmpeg-Builds/releases/download/" .. FFMPEG_AUTOBUILD_TAG .. "/ffmpeg-n" .. FFMPEG_BUILD_SUFFIX .. "-win64-lgpl-shared-$(version).zip")
    add_versions("7.1", "5d8d4378582bf75039e9d114d345ae73e44cd5fe674c9b0d52ccf05a8ad7c68b")
    on_load("windows", function (package)
        package:add("links", "avcodec", "avformat", "avutil", "swresample", "swscale")
        package:add("includedirs", "include")
        package:add("linkdirs", "lib")
        package:add("bindirs", "bin")
    end)
    on_install("windows", function (package)
        local version = package:version_str()
        local root = "ffmpeg-n" .. FFMPEG_BUILD_SUFFIX .. "-win64-lgpl-shared-" .. version
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
