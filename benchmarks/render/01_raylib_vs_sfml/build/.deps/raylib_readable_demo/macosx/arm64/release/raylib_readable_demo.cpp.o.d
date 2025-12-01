{
    depfiles_format = "gcc",
    files = {
        "raylib_readable_demo.cpp"
    },
    values = {
        "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++",
        {
            "-Qunused-arguments",
            "-target",
            "arm64-apple-macos26.0",
            "-isysroot",
            "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX26.0.sdk",
            "-fvisibility=hidden",
            "-fvisibility-inlines-hidden",
            "-O3",
            "-std=c++20",
            "-DGL_SILENCE_DEPRECATION",
            "-framework",
            "CoreVideo",
            "-framework",
            "CoreGraphics",
            "-framework",
            "AppKit",
            "-framework",
            "IOKit",
            "-framework",
            "CoreFoundation",
            "-framework",
            "Foundation",
            "-framework",
            "OpenGL",
            "-isystem",
            "/Users/dydy2brazil/.xmake/packages/r/raylib/5.5/d7b400902f2b425cafe86ca2bb148341/include",
            "-DNDEBUG"
        }
    },
    depfiles = "build/.objs/raylib_readable_demo/macosx/arm64/release/__cpp_raylib_readable_demo.cpp.cpp:   raylib_readable_demo.cpp\
"
}