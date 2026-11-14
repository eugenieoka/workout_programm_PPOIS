add_executable(workout_tests
    UserTest.cpp
    ExerciseTest.cpp
    UserJsonTest.cpp
    ExerciseJsonTest.cpp
    WorkoutPlanTest.cpp
    WorkoutPlanJsonTest.cpp
    PlanGeneratorTest.cpp
)

target_link_libraries(workout_tests
    PRIVATE
        workout_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(workout_tests)

add_executable(workout_db_tests
    DatabaseTest.cpp
)

target_link_libraries(workout_db_tests
    PRIVATE
        workout_db
        GTest::gtest_main
)

target_compile_definitions(workout_db_tests
    PRIVATE WORKOUT_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

gtest_discover_tests(workout_db_tests)
