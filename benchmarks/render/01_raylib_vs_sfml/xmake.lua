set_project("raylib_vs_sfml")
set_version("0.1.0")
set_languages("cxx20")
add_rules("mode.release", "mode.debug")

add_requires("raylib", "sfml 2.6.1")

set_policy("build.across_targets_in_parallel", true)

target("raylib_oop_bench")
    set_kind("binary")
    add_files("raylib_oop.cpp")
    add_packages("raylib")

target("raylib_ecs_bench")
    set_kind("binary")
    add_files("raylib_ecs.cpp")
    add_packages("raylib")

target("raylib_readable_demo")
    set_kind("binary")
    add_files("raylib_readable_demo.cpp")
    add_packages("raylib")

target("sfml_oop_bench")
    set_kind("binary")
    add_files("sfml_oop.cpp")
    add_packages("sfml")
    if is_plat("macosx") then
        add_ldflags("-ObjC")
    end

target("sfml_ecs_bench")
    set_kind("binary")
    add_files("sfml_ecs.cpp")
    add_packages("sfml")
    if is_plat("macosx") then
        add_ldflags("-ObjC")
    end

target("sfml_readable_demo")
    set_kind("binary")
    add_files("sfml_readable_demo.cpp")
    add_packages("sfml")
    if is_plat("macosx") then
        add_ldflags("-ObjC")
    end
