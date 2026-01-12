set_project("r-type")
set_version("0.1.0")
set_xmakever("3.0.6")

add_rules("mode.debug", "mode.release")
set_languages("cxx23")
set_warnings("all")
set_policy("package.requires_lock", true)

if is_plat("windows") then
	set_toolchains("clang-cl")
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
