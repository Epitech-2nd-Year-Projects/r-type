set_project("r-type")
set_version("0.1.0")
set_xmakever("2.8.5")

add_rules("mode.debug", "mode.release")
set_languages("cxx23")
set_warnings("all")
set_policy("package.requires_lock", true)

if is_plat("windows") then
	set_toolset("ar", "llvm-ar")
end

includes("engine", "server", "client", "protocol", "game_logic")

target("r-type")
set_kind("phony")
set_default(false)
add_deps("engine", "server", "client", "protocol", "game_logic")
