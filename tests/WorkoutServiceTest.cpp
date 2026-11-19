#include <gtest/gtest.h>

#include "app/ServiceError.h"
#include "app/WorkoutService.h"
#include "db/Database.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifndef WORKOUT_DATA_DIR
#define WORKOUT_DATA_DIR "."
#endif

namespace workout::app {
namespace {

constexpr uint16_t kAllEquipmentBits = 0x07FF;

class WorkoutServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_unique<infra::Database>(":memory:");
        db->executeFile(std::string(WORKOUT_DATA_DIR) + "/schema.sql");
        db->executeFile(std::string(WORKOUT_DATA_DIR) + "/seed.sql");
        service = std::make_unique<WorkoutService>(*db);
    }

    core::UserConstraints constraints() const {
        core::UserConstraints result;
        result.availableEquipment =
            static_cast<core::Equipment>(kAllEquipmentBits);
        return result;
    }

    core::UserProfile makeProfile(const std::string& name,
                                  core::TrainingGoal goal,
                                  core::TrainingFrequency frequency,
                                  core::UserConstraints constraints) {
        const auto profile = core::UserProfile::create(
            name, 30, core::Gender::Male, 180.0, 80.0,
            core::ExperienceLevel::Intermediate, goal, frequency,
            std::move(constraints), -1.0, 24);
        if (!profile) {
            throw std::runtime_error("test profile is invalid");
        }
        return *profile;
    }

    core::UserProfile makeProfile(const std::string& name,
                                  core::TrainingGoal goal,
                                  core::TrainingFrequency frequency) {
        return makeProfile(name, goal, frequency, constraints());
    }

    std::pair<int, int> firstExercise(const PlanInfo& plan) {
        for (const PlanDayInfo& day : plan.days) {
            if (!day.exercises.empty()) {
                return {day.dayIndex, day.exercises.front().position};
            }
        }
        throw std::runtime_error("plan has no exercises");
    }

    std::unique_ptr<infra::Database> db;
    std::unique_ptr<WorkoutService> service;
};

TEST_F(WorkoutServiceTest, ProfileCrud) {
    const core::UserProfile profile =
        makeProfile("Ivan", core::TrainingGoal::Strength, core::TrainingFrequency::Days3);
    const int64_t id = service->saveProfile(profile);
    ASSERT_GT(id, 0);

    auto loaded = service->getProfile(id);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->getName(), "Ivan");
    EXPECT_EQ(loaded->getGoal(), core::TrainingGoal::Strength);
    EXPECT_EQ(loaded->getFrequency(), core::TrainingFrequency::Days3);
    EXPECT_EQ(loaded->getConstraints().availableEquipment,
              static_cast<core::Equipment>(kAllEquipmentBits));

    loaded = loaded->withBodyWeight(82.5);
    EXPECT_EQ(service->saveProfile(*loaded), id);
    EXPECT_DOUBLE_EQ(service->getProfile(id)->getBodyMetrics().weightKg, 82.5);

    ASSERT_EQ(service->listProfiles().size(), 1u);
    EXPECT_TRUE(service->deleteProfile(id));
    EXPECT_FALSE(service->getProfile(id).has_value());
    EXPECT_TRUE(service->listProfiles().empty());
}

TEST_F(WorkoutServiceTest, OneRepMaxCrud) {
    const int64_t id = service->saveProfile(
        makeProfile("Ivan", core::TrainingGoal::Strength,
                    core::TrainingFrequency::Days3));

    service->setOneRepMax(id, 1, 100.0);
    auto oneRepMaxes = service->getOneRepMaxes(id);
    ASSERT_EQ(oneRepMaxes.size(), 1u);
    EXPECT_DOUBLE_EQ(oneRepMaxes.at(1), 100.0);

    service->setOneRepMax(id, 1, 105.0);
    EXPECT_DOUBLE_EQ(service->getOneRepMaxes(id).at(1), 105.0);

    service->removeOneRepMax(id, 1);
    EXPECT_TRUE(service->getOneRepMaxes(id).empty());
}

TEST_F(WorkoutServiceTest, OneRepMaxErrorsUseServiceCodes) {
    const int64_t id = service->saveProfile(
        makeProfile("Ivan", core::TrainingGoal::Strength,
                    core::TrainingFrequency::Days3));

    try {
        service->setOneRepMax(id, 9999, 50.0);
        FAIL() << "expected ServiceError";
    } catch (const ServiceError& error) {
        EXPECT_EQ(error.code(), "EXERCISE_NOT_FOUND");
    }

    try {
        const auto values = service->getOneRepMaxes(9999);
        (void)values;
        FAIL() << "expected ServiceError";
    } catch (const ServiceError& error) {
        EXPECT_EQ(error.code(), "PROFILE_NOT_FOUND");
    }
}

TEST_F(WorkoutServiceTest, ExerciseCatalogFilters) {
    EXPECT_EQ(service->listExercises().size(), 27u);

    ExerciseFilter squatFilter;
    squatFilter.nameContains = "squat";
    const auto squats = service->listExercises(squatFilter);
    ASSERT_GE(squats.size(), 3u);
    for (const core::Exercise& exercise : squats) {
        EXPECT_NE(exercise.getName().find("Squat"), std::string::npos);
    }

    ExerciseFilter legFilter;
    legFilter.primaryMuscle = core::MuscleGroup::Legs_Quads;
    for (const core::Exercise& exercise : service->listExercises(legFilter)) {
        EXPECT_EQ(exercise.getPrimaryMuscle(), core::MuscleGroup::Legs_Quads);
    }

    ExerciseFilter barbellFilter;
    barbellFilter.equipment = core::Equipment::Barbell;
    for (const core::Exercise& exercise : service->listExercises(barbellFilter)) {
        EXPECT_TRUE(core::hasEquipment(exercise.getRequiredEquipment(),
                                       core::Equipment::Barbell));
    }

    ExerciseFilter cardioFilter;
    cardioFilter.kind = core::ExerciseKind::Cardio;
    EXPECT_EQ(service->listExercises(cardioFilter).size(), 2u);

    ExerciseFilter easyFilter;
    easyFilter.maxDifficulty = 1;
    for (const core::Exercise& exercise : service->listExercises(easyFilter)) {
        EXPECT_LE(exercise.getDifficulty(), 1);
    }

    ExerciseFilter isolationFilter;
    isolationFilter.structure = core::ExerciseStructure::Isolation;
    for (const core::Exercise& exercise : service->listExercises(isolationFilter)) {
        EXPECT_EQ(exercise.getStructure(), core::ExerciseStructure::Isolation);
    }
}

TEST_F(WorkoutServiceTest, FindCompatibleRespectsConstraints) {
    core::UserConstraints constrained = constraints();
    constrained.userAvoidOverheadMotion = true;
    constrained.availableEquipment = core::Equipment::Dumbbells;

    const auto compatible = service->findCompatible(constrained);
    ASSERT_FALSE(compatible.empty());
    for (const core::Exercise& exercise : compatible) {
        EXPECT_FALSE(exercise.hasOverheadMotion());
        EXPECT_TRUE(core::hasEquipment(core::Equipment::Dumbbells,
                                       exercise.getRequiredEquipment()));
    }
}

TEST_F(WorkoutServiceTest, GeneratePlanForGoalsAndFrequencies) {
    const std::vector<core::TrainingGoal> goals = {
        core::TrainingGoal::Strength,
        core::TrainingGoal::Hypertrophy,
        core::TrainingGoal::WeightLoss,
        core::TrainingGoal::Maintenance,
    };
    const std::vector<core::TrainingFrequency> frequencies = {
        core::TrainingFrequency::Days3,
        core::TrainingFrequency::Days5,
        core::TrainingFrequency::Days6,
    };

    for (const core::TrainingGoal goal : goals) {
        for (const core::TrainingFrequency frequency : frequencies) {
            const core::UserProfile profile =
                makeProfile("Plan User", goal, frequency);
            const core::WorkoutPlan plan = service->generatePlan(profile);
            EXPECT_FALSE(plan.isEmpty())
                << "goal=" << static_cast<int>(goal)
                << " frequency=" << static_cast<int>(frequency);
            EXPECT_EQ(plan.getDayCount(), static_cast<std::size_t>(frequency));
        }
    }
}

TEST_F(WorkoutServiceTest, SaveLoadAndUpdatePlan) {
    const int64_t userId = service->saveProfile(
        makeProfile("Ivan", core::TrainingGoal::Strength,
                    core::TrainingFrequency::Days3));
    const auto profile = service->getProfile(userId);
    ASSERT_TRUE(profile.has_value());

    const core::WorkoutPlan plan = service->generatePlan(*profile);
    ASSERT_FALSE(plan.isEmpty());

    const int64_t planId = service->saveGeneratedPlan(
        *profile, plan, "Three Day Strength", true);
    ASSERT_GT(planId, 0);

    auto info = service->getPlan(planId);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, "Three Day Strength");
    EXPECT_EQ(info->goal, core::TrainingGoal::Strength);
    EXPECT_EQ(info->daysPerWeek, 3);
    EXPECT_TRUE(info->isActive);
    ASSERT_EQ(info->days.size(), plan.getDayCount());

    for (std::size_t dayIndex = 0; dayIndex < info->days.size(); ++dayIndex) {
        EXPECT_EQ(info->days[dayIndex].dayIndex, static_cast<int>(dayIndex));
        for (std::size_t position = 0;
             position < info->days[dayIndex].exercises.size(); ++position) {
            const PlanExerciseInfo& exercise = info->days[dayIndex].exercises[position];
            EXPECT_EQ(exercise.position, static_cast<int>(position));
            ASSERT_FALSE(exercise.exerciseName.empty());
            EXPECT_NE(exercise.exerciseName.front(), '#');
            EXPECT_FALSE(exercise.weightKg.has_value());
        }
    }

    ASSERT_EQ(service->listPlans(userId).size(), 1u);

    const auto [dayIndex, position] = firstExercise(*info);
    ASSERT_TRUE(service->updatePlanExercise(
        planId, dayIndex, position, 5, 3, 120, 100.0));

    const auto updated = service->getPlan(planId);
    ASSERT_TRUE(updated.has_value());
    EXPECT_TRUE(updated->isActive);

    bool found = false;
    for (const PlanDayInfo& day : updated->days) {
        if (day.dayIndex != dayIndex) {
            continue;
        }
        for (const PlanExerciseInfo& exercise : day.exercises) {
            if (exercise.position == position) {
                found = true;
                EXPECT_EQ(exercise.sets, 5);
                EXPECT_EQ(exercise.reps, 3);
                EXPECT_EQ(exercise.restSec, 120);
                ASSERT_TRUE(exercise.weightKg.has_value());
                EXPECT_DOUBLE_EQ(*exercise.weightKg, 100.0);
            }
        }
    }
    EXPECT_TRUE(found);

    EXPECT_FALSE(service->updatePlanExercise(
        planId, 99, position, 5, 3, 120, 100.0));
    EXPECT_FALSE(service->updatePlanExercise(
        planId, dayIndex, 99, 5, 3, 120, 100.0));
    EXPECT_THROW(service->updatePlanExercise(
                     planId, dayIndex, position, 0, 3, 120, 100.0),
                 std::invalid_argument);
}

TEST_F(WorkoutServiceTest, ActivateAndDeletePlan) {
    const int64_t userId = service->saveProfile(
        makeProfile("Ivan", core::TrainingGoal::Strength,
                    core::TrainingFrequency::Days3));
    const auto profile = service->getProfile(userId);
    ASSERT_TRUE(profile.has_value());

    const core::WorkoutPlan plan = service->generatePlan(*profile);
    ASSERT_FALSE(plan.isEmpty());

    const int64_t firstId = service->saveGeneratedPlan(
        *profile, plan, "First", true);
    const int64_t secondId = service->saveGeneratedPlan(
        *profile, plan, "Second", false);
    ASSERT_GT(firstId, 0);
    ASSERT_GT(secondId, 0);

    EXPECT_TRUE(service->getPlan(firstId)->isActive);
    EXPECT_FALSE(service->getPlan(secondId)->isActive);

    EXPECT_TRUE(service->activatePlan(secondId));
    EXPECT_FALSE(service->getPlan(firstId)->isActive);
    EXPECT_TRUE(service->getPlan(secondId)->isActive);

    EXPECT_TRUE(service->deletePlan(secondId));
    EXPECT_FALSE(service->getPlan(secondId).has_value());
    EXPECT_EQ(service->listPlans(userId).size(), 1u);
}

TEST_F(WorkoutServiceTest, DeleteProfileCascadesPlans) {
    const int64_t userId = service->saveProfile(
        makeProfile("Ivan", core::TrainingGoal::Strength,
                    core::TrainingFrequency::Days3));
    const auto profile = service->getProfile(userId);
    ASSERT_TRUE(profile.has_value());

    const core::WorkoutPlan plan = service->generatePlan(*profile);
    ASSERT_FALSE(plan.isEmpty());
    const int64_t planId = service->saveGeneratedPlan(
        *profile, plan, "Cascade", true);
    ASSERT_GT(planId, 0);

    EXPECT_TRUE(service->deleteProfile(userId));
    EXPECT_FALSE(service->getPlan(planId).has_value());
    EXPECT_TRUE(service->listPlans(userId).empty());
}

TEST_F(WorkoutServiceTest, SavePlanRejectsInvalidInput) {
    const core::UserProfile unsaved =
        makeProfile("Ivan", core::TrainingGoal::Strength,
                    core::TrainingFrequency::Days3);
    const core::WorkoutPlan plan = service->generatePlan(unsaved);
    ASSERT_FALSE(plan.isEmpty());

    try {
        service->saveGeneratedPlan(unsaved, plan, "Unsaved profile", false);
        FAIL() << "expected ServiceError";
    } catch (const ServiceError& error) {
        EXPECT_EQ(error.code(), "INVALID_ARGUMENT");
    }

    const int64_t userId = service->saveProfile(unsaved);
    const auto saved = service->getProfile(userId);
    ASSERT_TRUE(saved.has_value());

    try {
        service->saveGeneratedPlan(*saved, core::WorkoutPlan{}, "Empty", false);
        FAIL() << "expected ServiceError";
    } catch (const ServiceError& error) {
        EXPECT_EQ(error.code(), "EMPTY_PLAN");
    }
}

} // namespace
} // namespace workout::app
