cmake_minimum_required(VERSION 3.16)
project(xmake_vs_cmake_poc LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(fmt CONFIG REQUIRED)

add_executable(build_poc
    src/main.cpp
    src/foo.cpp
)
target_include_directories(build_poc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(build_poc PRIVATE fmt::fmt)
