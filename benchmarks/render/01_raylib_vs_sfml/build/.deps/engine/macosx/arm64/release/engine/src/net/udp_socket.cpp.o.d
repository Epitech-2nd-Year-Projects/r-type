{
    depfiles_format = "gcc",
    files = {
        "engine/src/net/udp_socket.cpp"
    },
    depfiles = "benchmarks/render/01_raylib_vs_sfml/build/.objs/engine/macosx/arm64/release/engine/src/net/__cpp_udp_socket.cpp.cpp:   engine/src/net/udp_socket.cpp   engine/src/net/../../include/engine/net/udp_socket.h   engine/src/net/../../include/engine/net/endpoint.h\
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