cmake_minimum_required(VERSION 3.16)
project(hw6)

set(CMAKE_CXX_STANDARD 17)

add_executable(hw6 source/main.cpp
        source/scene.cpp
        source/scene.h
        source/color.cpp
        source/color.h
        source/point.h
        source/figure.cpp
        source/figure.h
        source/rotation.h
        source/distribution.cpp
        source/distribution.h
        source/bvh.h
        source/gltf.h
        source/transform.h
        source/material.h
)
target_include_directories(hw6 PUBLIC rapidjson)
find_package(OpenMP)
target_link_libraries(hw6 OpenMP::OpenMP_CXX)
