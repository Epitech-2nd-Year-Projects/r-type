add_requires("benchmark")

target("storage_comparison_poc")
    set_kind("binary")
    add_files("main.cpp")
    add_includedirs(".")
    set_languages("cxx20")

target("storage_comparison_benchmark")
    set_kind("binary")
    add_files("benchmark.cpp")
    add_includedirs(".")
    add_packages("benchmark")
    set_languages("cxx20")