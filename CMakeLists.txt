cmake_minimum_required(VERSION 3.16)
project(workout_programm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(workout_core STATIC
    src/core/User.cpp
    src/core/Exercise.cpp
)

target_include_directories(workout_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
