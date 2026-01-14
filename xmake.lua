set_project("r-type")
set_version("0.1.0")
set_xmakever("3.0.6")

add_rules("mode.debug", "mode.release")
set_languages("cxx23")
set_warnings("all")
set_policy("package.requires_lock", true)

-- FORCE resolve conflict between raylib's dependency on libxrender 0.9.10 and others
-- Must be defined BEFORE add_requires
add_requireconfs("**.libxrender", {override = true, version = "0.9.12"})
add_requireconfs("**.libxext", {override = true, version = "1.3.6"})

add_requires("lz4")

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
