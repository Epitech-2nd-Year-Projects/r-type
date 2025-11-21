add_executable(storage_comparison_poc
        main.cpp
)
target_include_directories(storage_comparison_poc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(storage_comparison_benchmark
        benchmark.cpp
)
target_include_directories(storage_comparison_benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(storage_comparison_benchmark PRIVATE benchmark::benchmark benchmark::benchmark_main)