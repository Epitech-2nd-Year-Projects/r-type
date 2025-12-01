{
    depfiles_format = "gcc",
    files = {
        "protocol/src/world_snapshot.cpp"
    },
    depfiles = "benchmarks/render/01_raylib_vs_sfml/build/.objs/protocol/macosx/arm64/release/protocol/src/__cpp_world_snapshot.cpp.cpp:   protocol/src/world_snapshot.cpp   protocol/include/protocol/world_snapshot.h   engine/include/engine/net/packet_buffer.h\
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
            "-Iprotocol/include",
            "-Iengine/include",
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
            "-DNDEBUG"
        }
    }
}