set_project("xmake_vs_cmake_poc")
set_version("0.1.0")
set_languages("cxx20")
add_rules("mode.release", "mode.debug")

add_requires("fmt")
set_policy("build.across_targets_in_parallel", true)

target("build_poc")
    set_kind("binary")
    add_files("src/main.cpp", "src/foo.cpp")
    add_includedirs("src")
    add_packages("fmt")
