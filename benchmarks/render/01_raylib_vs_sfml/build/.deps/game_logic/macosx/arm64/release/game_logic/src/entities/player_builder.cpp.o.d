{
    depfiles_format = "gcc",
    files = {
        "game_logic/src/entities/player_builder.cpp"
    },
    depfiles = "benchmarks/render/01_raylib_vs_sfml/build/.objs/game_logic/macosx/arm64/release/game_logic/src/entities/__cpp_player_builder.cpp.cpp:   game_logic/src/entities/player_builder.cpp   game_logic/include/game_logic/entities/player_builder.h   engine/include/engine/ecs/entity_id.h   engine/include/engine/ecs/registry.h   engine/include/engine/ecs/sparse_array.h   engine/include/engine/ecs/system.h   engine/include/engine/time/time_delta.h   engine/include/engine/ecs/system_scheduler.h   engine/include/engine/math/vector2.h   engine/include/engine/ecs/component.h   engine/include/engine/ecs/components/bounding_box_component.h   engine/include/engine/math/rect.h   engine/include/engine/ecs/components/circle_collider_component.h   engine/include/engine/ecs/components/lifetime_component.h   engine/include/engine/ecs/components/position_component.h   engine/include/engine/ecs/components/tag_component.h   engine/include/engine/ecs/components/transform_component.h   engine/include/engine/math/transform.h   engine/include/engine/math/constants.h   engine/include/engine/ecs/components/velocity_component.h   game_logic/include/game_logic/components.h   game_logic/include/game_logic/components/ai_component.h   game_logic/include/game_logic/components/animation_component.h   game_logic/include/game_logic/components/damageable_component.h   game_logic/include/game_logic/components/health_component.h   game_logic/include/game_logic/components/player_component.h   game_logic/include/game_logic/components/powerup_component.h   game_logic/include/game_logic/components/score_value_component.h   game_logic/include/game_logic/components/sprite_component.h   game_logic/include/game_logic/components/weapon_component.h\
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
            "-Igame_logic/include",
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