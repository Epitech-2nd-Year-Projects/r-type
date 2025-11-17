{
    files = {
        "client/src/main.cpp"
    },
    depfiles_format = "gcc",
    depfiles = "build/.objs/client/macosx/arm64/release/client/src/__cpp_main.cpp.cpp:   client/src/main.cpp\
",
    values = {
        "/Library/Developer/CommandLineTools/usr/bin/clang++",
        {
            "-Qunused-arguments",
            "-isysroot",
            "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk",
            "-fvisibility=hidden",
            "-fvisibility-inlines-hidden",
            "-Wall",
            "-O3",
            "-std=c++23",
            "-DNDEBUG"
        }
    }
}