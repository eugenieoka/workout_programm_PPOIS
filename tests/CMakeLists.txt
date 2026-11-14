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
