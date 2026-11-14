#include "core/WorkoutPlan.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace workout::core;

WorkoutPlan makePlan() {
    return WorkoutPlan{std::vector<PlanDay>{
        {1, "Upper A", {{10, 4, 10, 135}, {20, 3, 12, 90}}},
        {2, "Lower A", {{30, 4, 5, 270}}}
    }};
}

} // namespace

TEST(WorkoutPlanJson, SerializesAllFields) {
    const std::string text = makePlan().toJson();

    EXPECT_NE(text.find("\"Days\": ["), std::string::npos);
    EXPECT_NE(text.find("\"DayIndex\": 1"), std::string::npos);
    EXPECT_NE(text.find("\"Name\": \"Upper A\""), std::string::npos);
    EXPECT_NE(text.find("\"ExerciseId\": 10"), std::string::npos);
    EXPECT_NE(text.find("\"Sets\": 4"), std::string::npos);
    EXPECT_NE(text.find("\"Reps\": 10"), std::string::npos);
    EXPECT_NE(text.find("\"RestSec\": 135"), std::string::npos);
    EXPECT_NE(text.find("\"Name\": \"Lower A\""), std::string::npos);
    EXPECT_NE(text.find("\"RestSec\": 270"), std::string::npos);
}

TEST(WorkoutPlanJson, RoundTrips) {
    const WorkoutPlan plan = makePlan();
    const std::string text = plan.toJson();

    const WorkoutPlan restored = WorkoutPlan::fromJson(text);

    EXPECT_EQ(restored.getDayCount(), plan.getDayCount());
    ASSERT_EQ(restored.getDays().size(), plan.getDays().size());

    for (std::size_t i = 0; i < plan.getDays().size(); ++i) {
        EXPECT_EQ(restored.getDays()[i].dayIndex, plan.getDays()[i].dayIndex);
        EXPECT_EQ(restored.getDays()[i].name, plan.getDays()[i].name);
        ASSERT_EQ(restored.getDays()[i].exercises.size(),
                  plan.getDays()[i].exercises.size());

        for (std::size_t j = 0; j < plan.getDays()[i].exercises.size(); ++j) {
            EXPECT_EQ(restored.getDays()[i].exercises[j].exerciseId,
                      plan.getDays()[i].exercises[j].exerciseId);
            EXPECT_EQ(restored.getDays()[i].exercises[j].sets,
                      plan.getDays()[i].exercises[j].sets);
            EXPECT_EQ(restored.getDays()[i].exercises[j].reps,
                      plan.getDays()[i].exercises[j].reps);
            EXPECT_EQ(restored.getDays()[i].exercises[j].restSec,
                      plan.getDays()[i].exercises[j].restSec);
        }
    }

    EXPECT_EQ(restored.toJson(), text);
}

TEST(WorkoutPlanJson, RoundTripsEmptyPlan) {
    const WorkoutPlan plan;
    const std::string text = plan.toJson();

    EXPECT_NE(text.find("\"Days\": []"), std::string::npos);

    const WorkoutPlan restored = WorkoutPlan::fromJson(text);
    EXPECT_TRUE(restored.isEmpty());
    EXPECT_EQ(restored.getDayCount(), 0u);
}

TEST(WorkoutPlanJson, AcceptsBoundaryValues) {
    const WorkoutPlan plan{std::vector<PlanDay>{
        {1, "Boundary", {{1, 1, 1, 0}, {2, 20, 100, 3600}}}
    }};

    const WorkoutPlan restored = WorkoutPlan::fromJson(plan.toJson());
    ASSERT_EQ(restored.getDayCount(), 1u);
    ASSERT_EQ(restored.getDays()[0].exercises.size(), 2u);
    EXPECT_EQ(restored.getDays()[0].exercises[0].sets, 1);
    EXPECT_EQ(restored.getDays()[0].exercises[0].reps, 1);
    EXPECT_EQ(restored.getDays()[0].exercises[0].restSec, 0);
    EXPECT_EQ(restored.getDays()[0].exercises[1].sets, 20);
    EXPECT_EQ(restored.getDays()[0].exercises[1].reps, 100);
    EXPECT_EQ(restored.getDays()[0].exercises[1].restSec, 3600);
}

TEST(WorkoutPlanJson, RejectsMalformedJson) {
    EXPECT_THROW(WorkoutPlan::fromJson("{"), std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson("[]"), std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson("{}"), std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(R"({"Days":{}})"), std::runtime_error);
}

TEST(WorkoutPlanJson, RejectsInvalidDays) {
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":0,"Name":"A","Exercises":[]}]})"),
                 std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":" ","Exercises":[]}]})"),
                 std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":"A","Exercises":{}}]})"),
                 std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":"A","Exercises":[]},{"DayIndex":1,"Name":"B","Exercises":[]}]})"),
                 std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":"A"}]})"),
                 std::runtime_error);
}

TEST(WorkoutPlanJson, RejectsInvalidExercises) {
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":"A","Exercises":[{"ExerciseId":0,"Sets":3,"Reps":10,"RestSec":90}]}]})"),
                 std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":"A","Exercises":[{"ExerciseId":1,"Sets":0,"Reps":10,"RestSec":90}]}]})"),
                 std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":"A","Exercises":[{"ExerciseId":1,"Sets":21,"Reps":10,"RestSec":90}]}]})"),
                 std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":"A","Exercises":[{"ExerciseId":1,"Sets":3,"Reps":0,"RestSec":90}]}]})"),
                 std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":"A","Exercises":[{"ExerciseId":1,"Sets":3,"Reps":101,"RestSec":90}]}]})"),
                 std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":"A","Exercises":[{"ExerciseId":1,"Sets":3,"Reps":10,"RestSec":-1}]}]})"),
                 std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":"A","Exercises":[{"ExerciseId":1,"Sets":3,"Reps":10,"RestSec":3601}]}]})"),
                 std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":"A","Exercises":[{"ExerciseId":1,"Sets":3,"Reps":10}]}]})"),
                 std::runtime_error);
    EXPECT_THROW(WorkoutPlan::fromJson(
                     R"({"Days":[{"DayIndex":1,"Name":"A","Exercises":[[]]}]})"),
                 std::runtime_error);
}
