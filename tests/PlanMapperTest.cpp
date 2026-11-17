#include <gtest/gtest.h>

#include "app/PlanMapper.h"
#include "core/PlanGenerator.h"
#include "db/Database.h"
#include "db/ExerciseRepository.h"
#include "db/PlanRepository.h"
#include "db/UserRepository.h"

#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#ifndef WORKOUT_DATA_DIR
#define WORKOUT_DATA_DIR "."
#endif

namespace workout::app {
namespace {

constexpr uint16_t kAllEquipmentBits = 0x07FF;

class PlanMapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_unique<infra::Database>(":memory:");
        db->executeFile(std::string(WORKOUT_DATA_DIR) + "/schema.sql");
        db->executeFile(std::string(WORKOUT_DATA_DIR) + "/seed.sql");
    }

    core::UserProfile makeProfile(core::TrainingGoal goal,
                                  core::TrainingFrequency frequency) {
        core::UserConstraints constraints;
        constraints.availableEquipment =
            static_cast<core::Equipment>(kAllEquipmentBits);

        const auto profile = core::UserProfile::create(
            "Mapper User", 30, core::Gender::Male, 180.0, 80.0,
            core::ExperienceLevel::Intermediate, goal, frequency,
            constraints, -1.0, 24);
        if (!profile) {
            throw std::runtime_error("test profile is invalid");
        }
        return *profile;
    }

    core::WorkoutPlan generate(const core::UserProfile& profile) {
        const auto catalog =
            infra::ExerciseRepository(*db).findCompatible(profile.getConstraints());
        return core::PlanGenerator::generate(profile, catalog);
    }

    std::map<core::ExerciseId, std::string> exerciseNames() {
        std::map<core::ExerciseId, std::string> names;
        for (const core::Exercise& exercise : infra::ExerciseRepository(*db).findAll()) {
            names.emplace(exercise.getId(), exercise.getName());
        }
        return names;
    }

    std::unique_ptr<infra::Database> db;
};

TEST_F(PlanMapperTest, FromGeneratedPlanWritesZeroBasedIndices) {
    const core::UserProfile profile =
        makeProfile(core::TrainingGoal::Strength, core::TrainingFrequency::Days3);
    const core::WorkoutPlan plan = generate(profile);
    ASSERT_FALSE(plan.isEmpty());

    const int64_t userId = infra::UserRepository(*db).save(profile);
    const infra::PlanRecord record = PlanMapper::fromGeneratedPlan(
        plan, userId, "Three Day Strength", core::TrainingGoal::Strength, 3, true);

    EXPECT_EQ(record.userId, userId);
    EXPECT_EQ(record.name, "Three Day Strength");
    EXPECT_EQ(record.goal, core::TrainingGoal::Strength);
    EXPECT_EQ(record.daysPerWeek, 3);
    EXPECT_TRUE(record.isActive);
    ASSERT_EQ(record.days.size(), plan.getDayCount());

    for (std::size_t dayIndex = 0; dayIndex < record.days.size(); ++dayIndex) {
        EXPECT_EQ(record.days[dayIndex].dayIndex, static_cast<int>(dayIndex));
        EXPECT_EQ(record.days[dayIndex].name, plan.getDays()[dayIndex].name);
        ASSERT_EQ(record.days[dayIndex].exercises.size(),
                  plan.getDays()[dayIndex].exercises.size());

        for (std::size_t position = 0;
             position < record.days[dayIndex].exercises.size(); ++position) {
            const infra::PlanExerciseRecord& exercise =
                record.days[dayIndex].exercises[position];
            const core::PlannedExercise& source =
                plan.getDays()[dayIndex].exercises[position];

            EXPECT_EQ(exercise.position, static_cast<int>(position));
            EXPECT_EQ(exercise.exerciseId, source.exerciseId);
            EXPECT_EQ(exercise.sets, source.sets);
            EXPECT_EQ(exercise.reps, source.reps);
            EXPECT_EQ(exercise.restSec, source.restSec);
            EXPECT_FALSE(exercise.weightKg.has_value());
            EXPECT_TRUE(exercise.notes.empty());
        }
    }
}

TEST_F(PlanMapperTest, RoundTripPreservesOrderAndNames) {
    const core::UserProfile profile =
        makeProfile(core::TrainingGoal::Hypertrophy, core::TrainingFrequency::Days5);
    const core::WorkoutPlan plan = generate(profile);
    ASSERT_FALSE(plan.isEmpty());

    const int64_t userId = infra::UserRepository(*db).save(profile);
    infra::PlanRecord record = PlanMapper::fromGeneratedPlan(
        plan, userId, "Five Day Hypertrophy", core::TrainingGoal::Hypertrophy, 5, true);

    infra::PlanRepository plans(*db);
    const int64_t planId = plans.save(record);
    ASSERT_GT(planId, 0);

    const auto loaded = plans.findById(planId);
    ASSERT_TRUE(loaded.has_value());
    const auto names = exerciseNames();
    const PlanInfo info = PlanMapper::toPlanInfo(*loaded, names);

    EXPECT_EQ(info.id, planId);
    EXPECT_EQ(info.userId, userId);
    EXPECT_EQ(info.name, "Five Day Hypertrophy");
    EXPECT_EQ(info.goal, core::TrainingGoal::Hypertrophy);
    EXPECT_EQ(info.daysPerWeek, 5);
    EXPECT_TRUE(info.isActive);
    ASSERT_EQ(info.days.size(), loaded->days.size());

    for (std::size_t dayIndex = 0; dayIndex < info.days.size(); ++dayIndex) {
        EXPECT_EQ(info.days[dayIndex].dayIndex, static_cast<int>(dayIndex));
        EXPECT_EQ(info.days[dayIndex].name, loaded->days[dayIndex].name);
        ASSERT_EQ(info.days[dayIndex].exercises.size(),
                  loaded->days[dayIndex].exercises.size());

        for (std::size_t position = 0;
             position < info.days[dayIndex].exercises.size(); ++position) {
            const PlanExerciseInfo& exercise = info.days[dayIndex].exercises[position];
            EXPECT_EQ(exercise.position, static_cast<int>(position));
            EXPECT_EQ(exercise.exerciseId,
                      loaded->days[dayIndex].exercises[position].exerciseId);
            const auto nameIt = names.find(exercise.exerciseId);
            ASSERT_NE(nameIt, names.end());
            EXPECT_EQ(exercise.exerciseName, nameIt->second);
            EXPECT_FALSE(exercise.weightKg.has_value());
            EXPECT_TRUE(exercise.notes.empty());
        }
    }
}

TEST_F(PlanMapperTest, ToPlanInfoFallsBackForUnknownExerciseName) {
    infra::PlanRecord record;
    record.id = 7;
    record.userId = 1;
    record.name = "Unknown";
    record.daysPerWeek = 1;

    infra::PlanDayRecord day;
    day.dayIndex = 0;
    day.name = "Day A";

    infra::PlanExerciseRecord exercise;
    exercise.exerciseId = 9999;
    exercise.position = 0;
    exercise.sets = 3;
    exercise.reps = 10;
    exercise.restSec = 60;
    day.exercises.push_back(exercise);
    record.days.push_back(day);

    const PlanInfo info = PlanMapper::toPlanInfo(record, {});
    ASSERT_EQ(info.days.size(), 1u);
    ASSERT_EQ(info.days[0].exercises.size(), 1u);
    EXPECT_EQ(info.days[0].exercises[0].exerciseName, "#9999");
}

TEST_F(PlanMapperTest, RejectsEmptyPlan) {
    EXPECT_THROW(
        PlanMapper::fromGeneratedPlan(core::WorkoutPlan{}, 1, "Empty",
                                      core::TrainingGoal::Strength, 3, false),
        std::invalid_argument);
}

TEST_F(PlanMapperTest, RejectsInvalidExercise) {
    core::PlanDay day;
    day.name = "Day A";
    core::PlannedExercise exercise;
    exercise.exerciseId = 0;
    exercise.sets = 3;
    exercise.reps = 10;
    exercise.restSec = 60;
    day.exercises.push_back(exercise);

    const core::WorkoutPlan plan({day});
    EXPECT_THROW(
        PlanMapper::fromGeneratedPlan(plan, 1, "Invalid",
                                      core::TrainingGoal::Strength, 1, false),
        std::invalid_argument);
}

} // namespace
} // namespace workout::app
