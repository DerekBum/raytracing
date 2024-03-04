cmake_minimum_required(VERSION 3.16)
project(hw2)

set(CMAKE_CXX_STANDARD 17)

add_executable(hw2 source/main.cpp
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
        source/light.cpp
        source/light.h)
