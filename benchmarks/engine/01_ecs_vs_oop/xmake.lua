add_requires("benchmark")

target("ecs_vs_oop_poc")
    set_kind("binary")
    add_files("main.cpp", "ecs.cpp", "oop.cpp")
    add_includedirs(".")
    set_languages("cxx20")

target("ecs_vs_oop_benchmark")
    set_kind("binary")
    add_files("benchmark.cpp", "ecs.cpp", "oop.cpp")
    add_includedirs(".")
    add_packages("benchmark")
    set_languages("cxx20")