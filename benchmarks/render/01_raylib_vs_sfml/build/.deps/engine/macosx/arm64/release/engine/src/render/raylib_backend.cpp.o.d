{
    depfiles_format = "gcc",
    files = {
        "engine/src/render/raylib_backend.cpp"
    },
    depfiles = "benchmarks/render/01_raylib_vs_sfml/build/.objs/engine/macosx/arm64/release/engine/src/render/__cpp_raylib_backend.cpp.cpp:   engine/src/render/raylib_backend.cpp   engine/include/engine/render/raylib_backend.h   engine/include/engine/render/backend.h   engine/include/engine/render/window.h   engine/include/engine/render/context.h   engine/include/engine/render/color.h   engine/include/engine/math/vector2.h   engine/include/engine/render/renderer2d.h   engine/include/engine/math/rect.h engine/include/engine/render/layer.h\
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