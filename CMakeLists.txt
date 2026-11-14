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

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest)

enable_testing()

find_package(SQLite3 REQUIRED)

add_library(workout_core STATIC
    src/core/User.cpp
    src/core/Exercise.cpp
    src/core/UserJson.cpp
    src/core/ExerciseJson.cpp
    src/core/WorkoutPlan.cpp
    src/core/WorkoutPlanJson.cpp
    src/core/PlanGenerator.cpp
)

target_include_directories(workout_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(workout_core
    PRIVATE nlohmann_json::nlohmann_json
)

add_library(workout_db STATIC
    src/db/Database.cpp
    src/db/UserRepository.cpp
    src/db/ExerciseRepository.cpp
    src/db/PlanRepository.cpp
)

target_include_directories(workout_db
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(workout_db
    PUBLIC
        SQLite::SQLite3
        workout_core
)

add_subdirectory(tests)
