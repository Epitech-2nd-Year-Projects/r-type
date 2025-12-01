{
    depfiles_format = "gcc",
    files = {
        "protocol/src/header.cpp"
    },
    depfiles = "benchmarks/render/01_raylib_vs_sfml/build/.objs/protocol/macosx/arm64/release/protocol/src/__cpp_header.cpp.cpp:   protocol/src/header.cpp protocol/include/protocol/header.h   protocol/include/protocol/message_type.h   engine/include/engine/net/packet_buffer.h\
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