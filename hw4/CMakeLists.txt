cmake_minimum_required(VERSION 3.16)
project(hw4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(hw4 source/main.cpp
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
)
target_link_libraries(hw4 Threads::Threads)
