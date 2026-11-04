cmake_minimum_required(VERSION 3.16)
project(workout_programm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(workout_core STATIC
    src/core/User.cpp
    src/core/Exercise.cpp
    src/core/UserJson.cpp
    src/core/ExerciseJson.cpp
)

target_include_directories(workout_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(workout_core
    PRIVATE nlohmann_json::nlohmann_json
)
