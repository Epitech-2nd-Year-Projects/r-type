set_project("r-type")
set_version("0.1.0")
set_xmakever("2.8.5")

add_rules("mode.debug", "mode.release")
set_languages("cxx23")
set_warnings("all")
set_policy("package.requires_lock", true)

if is_plat("windows") then
	set_toolchains("msvc")
	set_toolset("cc", "cl")
	set_toolset("cxx", "cl")
	set_toolset("ld", "link")
	set_toolset("sh", "link")
	set_toolset("ar", "lib")
end

includes("engine", "server", "client", "protocol", "game_logic")

target("r-type")
set_kind("phony")
set_default(false)
add_deps("engine", "server", "client", "protocol", "game_logic")

if os.getenv("BUILD_BENCHMARKS") then
    includes("benchmarks/engine")
end
