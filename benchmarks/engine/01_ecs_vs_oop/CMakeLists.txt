add_executable(ecs_vs_oop_poc
        oop.cpp
        ecs.cpp
        main.cpp
)
target_include_directories(ecs_vs_oop_poc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ecs_vs_oop_benchmark
        oop.cpp
        ecs.cpp
        benchmark.cpp
)
target_include_directories(ecs_vs_oop_benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ecs_vs_oop_benchmark PRIVATE benchmark::benchmark benchmark::benchmark_main)