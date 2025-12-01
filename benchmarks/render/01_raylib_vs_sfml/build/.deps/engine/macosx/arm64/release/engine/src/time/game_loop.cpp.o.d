{
    depfiles_format = "gcc",
    files = {
        "engine/src/time/game_loop.cpp"
    },
    depfiles = "benchmarks/render/01_raylib_vs_sfml/build/.objs/engine/macosx/arm64/release/engine/src/time/__cpp_game_loop.cpp.cpp:   engine/src/time/game_loop.cpp   engine/src/time/../../include/engine/time/game_loop.h   engine/src/time/../../include/engine/time/frame_timer.h   engine/src/time/../../include/engine/time/clock.h   engine/src/time/../../include/engine/time/time_delta.h\
",
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
            "-Wall",
            "-O3",
            "-std=c++23",
            "-Iengine/include",
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
            "/Users/dydy2brazil/.xmake/packages/a/asio/1.36.0/dfa7e54bb9ca472cb5b3ba496ed7a9ce/include",
            "-isystem",
            "/Users/dydy2brazil/.xmake/packages/r/raylib/5.5/d7b400902f2b425cafe86ca2bb148341/include",
            "-DNDEBUG"
        }
    }
}