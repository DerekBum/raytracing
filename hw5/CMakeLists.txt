cmake_minimum_required(VERSION 3.16)
project(hw5)

set(CMAKE_CXX_STANDARD 17)

add_executable(hw5 source/main.cpp
        source/scene.cpp
        source/scene.h
        source/color.cpp
        source/color.h
        source/point.cpp
        source/point.h
        source/figure.cpp
        source/figure.h
        source/rotation.cpp
        source/rotation.h
        source/distribution.cpp
        source/distribution.h
        source/bvh.cpp
        source/bvh.h
)
find_package(OpenMP)
target_link_libraries(hw5 OpenMP::OpenMP_CXX)
