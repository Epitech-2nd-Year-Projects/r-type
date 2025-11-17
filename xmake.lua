set_project("r-type")
set_version("0.1.0")
set_xmakever("2.8.5")

add_rules("mode.debug", "mode.release")
set_languages("cxx23")
set_warnings("all")
set_policy("package.requires_lock", true)

includes("shared", "engine", "server", "client", "tools")

target("r-type")
    set_kind("phony")
    set_default(false)
    add_deps("shared", "engine", "server", "client", "tools")
