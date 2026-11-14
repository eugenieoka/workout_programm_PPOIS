#include "core/WorkoutPlan.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

using namespace workout::core;

} // namespace

TEST(WorkoutPlanModel, DefaultsToEmpty) {
    const WorkoutPlan plan;

    EXPECT_TRUE(plan.isEmpty());
    EXPECT_EQ(plan.getDayCount(), 0u);
    EXPECT_TRUE(plan.getDays().empty());
}

TEST(WorkoutPlanModel, StoresDaysAndExercises) {
    const WorkoutPlan plan{std::vector<PlanDay>{
        {1, "Upper A", {{10, 4, 10, 135}, {20, 3, 12, 90}}},
        {2, "Lower A", {{30, 4, 5, 270}}}
    }};

    EXPECT_FALSE(plan.isEmpty());
    ASSERT_EQ(plan.getDayCount(), 2u);

    const auto& days = plan.getDays();
    EXPECT_EQ(days[0].dayIndex, 1);
    EXPECT_EQ(days[0].name, "Upper A");
    ASSERT_EQ(days[0].exercises.size(), 2u);
    EXPECT_EQ(days[0].exercises[0].exerciseId, 10);
    EXPECT_EQ(days[0].exercises[0].sets, 4);
    EXPECT_EQ(days[0].exercises[0].reps, 10);
    EXPECT_EQ(days[0].exercises[0].restSec, 135);
    EXPECT_EQ(days[0].exercises[1].exerciseId, 20);
    EXPECT_EQ(days[0].exercises[1].sets, 3);
    EXPECT_EQ(days[0].exercises[1].reps, 12);
    EXPECT_EQ(days[0].exercises[1].restSec, 90);

    EXPECT_EQ(days[1].dayIndex, 2);
    EXPECT_EQ(days[1].name, "Lower A");
    ASSERT_EQ(days[1].exercises.size(), 1u);
    EXPECT_EQ(days[1].exercises[0].exerciseId, 30);
    EXPECT_EQ(days[1].exercises[0].sets, 4);
    EXPECT_EQ(days[1].exercises[0].reps, 5);
    EXPECT_EQ(days[1].exercises[0].restSec, 270);
}
