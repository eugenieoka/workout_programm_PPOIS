#include <gtest/gtest.h>

#include "db/ExerciseRepository.h"
#include "db/PlanRepository.h"
#include "db/UserRepository.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef WORKOUT_DATA_DIR
#define WORKOUT_DATA_DIR "."
#endif

namespace workout::infra {
namespace {

class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_unique<Database>(":memory:");
        db->executeFile(std::string(WORKOUT_DATA_DIR) + "/schema.sql");
        db->executeFile(std::string(WORKOUT_DATA_DIR) + "/seed.sql");
    }

    int64_t createUser() {
        core::UserConstraints constraints;
        constraints.constraintsMask =
            core::HC_MASK(core::HealthConstraint::KneePain);
        constraints.availableEquipment =
            core::Equipment::Barbell | core::Equipment::PowerRack;
        constraints.userAvoidOverheadMotion = true;
        constraints.maxSessionMinutes = 60;
        constraints.customNotes = "test notes";

        const auto profile = core::UserProfile::create(
            "Test User", 30, core::Gender::Male, 180.0, 80.0,
            core::ExperienceLevel::Intermediate, core::TrainingGoal::Strength,
            core::TrainingFrequency::Days3, constraints, 15.0, 24);
        EXPECT_TRUE(profile.has_value());

        return UserRepository(*db).save(
            profile->withEstimated1RM(1, 100.0).withEstimated1RM(3, 80.0));
    }

    std::unique_ptr<Database> db;
};

TEST_F(DatabaseTest, SchemaAndSeedAreLoaded) {
    ExerciseRepository repo(*db);

    const auto exercises = repo.findAll();
    ASSERT_EQ(exercises.size(), 27u);

    const auto bench = repo.findByName("Bench Press");
    ASSERT_TRUE(bench.has_value());
    EXPECT_EQ(bench->getPrimaryMuscle(), core::MuscleGroup::Chest_Upper);
    EXPECT_EQ(bench->getRequiredEquipment(),
              core::Equipment::Barbell | core::Equipment::Bench);

    const auto squat = repo.findById(1);
    ASSERT_TRUE(squat.has_value());
    EXPECT_TRUE(squat->hasAxialLoading());
    EXPECT_TRUE(squat->hasDeepKneeFlexion());
    EXPECT_EQ(squat->getSecondaryMuscles().size(), 3u);
}

TEST_F(DatabaseTest, UserRoundTrip) {
    UserRepository repo(*db);
    const int64_t id = createUser();
    ASSERT_GT(id, 0);

    const auto loaded = repo.findById(id);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->getName(), "Test User");
    EXPECT_DOUBLE_EQ(loaded->getBodyMetrics().weightKg, 80.0);
    EXPECT_DOUBLE_EQ(loaded->getEstimated1RM(1), 100.0);
    EXPECT_DOUBLE_EQ(loaded->getEstimated1RM(3), 80.0);
    EXPECT_TRUE(loaded->getConstraints().userAvoidOverheadMotion);
    ASSERT_TRUE(loaded->getConstraints().maxSessionMinutes.has_value());
    EXPECT_EQ(*loaded->getConstraints().maxSessionMinutes, 60);
    EXPECT_EQ(loaded->getConstraints().customNotes, "test notes");

    // Reading does not change updatedAt.
    const auto updatedAt = loaded->getUpdatedAt();
    EXPECT_EQ(repo.findById(id)->getUpdatedAt(), updatedAt);

    // UPDATE of an existing profile.
    repo.save(loaded->withBodyWeight(82.5));
    EXPECT_DOUBLE_EQ(repo.findById(id)->getBodyMetrics().weightKg, 82.5);

    // 1RM: update and removal.
    repo.save(repo.findById(id)->withEstimated1RM(1, 105.0).withEstimated1RM(3, 0.0));
    EXPECT_DOUBLE_EQ(repo.findById(id)->getEstimated1RM(1), 105.0);
    EXPECT_DOUBLE_EQ(repo.findById(id)->getEstimated1RM(3), 0.0);

    EXPECT_EQ(repo.findAll().size(), 1u);
    EXPECT_TRUE(repo.remove(id));
    EXPECT_FALSE(repo.findById(id).has_value());
}

TEST_F(DatabaseTest, ExerciseSaveUpdatesAndRemoves) {
    ExerciseRepository repo(*db);

    auto created = core::Exercise::create(
        100, "Test Cable Fly", "description", core::MuscleGroup::Chest_Upper,
        {core::MuscleGroup::Chest_Lower, core::MuscleGroup::Shoulders_Front},
        core::ExerciseStructure::Isolation, core::ExerciseKind::Resistance,
        core::PurposeType::Accessory, core::MovementPattern::Push_Horizontal,
        core::Equipment::CableMachine, core::Equipment::Bench, 2, 3.5, 45,
        "instructions", "video");
    ASSERT_TRUE(created.has_value());
    EXPECT_EQ(repo.save(*created), 100);

    const auto loaded = repo.findById(100);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->getName(), "Test Cable Fly");
    EXPECT_EQ(loaded->getSecondaryMuscles().size(), 2u);
    EXPECT_EQ(loaded->getRequiredEquipment(), core::Equipment::CableMachine);

    // A second save -> UPDATE.
    const auto updated = core::Exercise::create(
        100, "Test Cable Fly 2", loaded->getDescription(), loaded->getPrimaryMuscle(),
        loaded->getSecondaryMuscles(), loaded->getStructure(), loaded->getKind(),
        loaded->getPurposeType(), loaded->getMovementPattern(),
        loaded->getRequiredEquipment(), loaded->getOptionalEquipment(),
        loaded->getDifficulty(), loaded->getMET(), loaded->getEstimatedSetDurationSec(),
        loaded->getInstructions(), loaded->getVideoUrl(), loaded->isPowerliftingLift(),
        loaded->hasAxialLoading(), loaded->hasSpinalFlexion(), loaded->hasOverheadMotion(),
        loaded->hasDeepKneeFlexion(), loaded->isUnilateral(),
        loaded->getRegressionId(), loaded->getProgressionId());
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(repo.save(*updated), 100);
    EXPECT_EQ(repo.findById(100)->getName(), "Test Cable Fly 2");

    EXPECT_TRUE(repo.remove(100));
    EXPECT_FALSE(repo.findById(100).has_value());
}

TEST_F(DatabaseTest, PlanRoundTripAndCascade) {
    const int64_t userId = createUser();
    PlanRepository repo(*db);

    PlanExerciseRecord squat;
    squat.exerciseId = 1;
    squat.sets = 5;
    squat.reps = 5;
    squat.restSec = 180;
    squat.weightKg = 100.0;

    PlanExerciseRecord bench;
    bench.exerciseId = 3;
    bench.sets = 4;
    bench.reps = 8;
    bench.restSec = 120;

    PlanDayRecord dayA;
    dayA.name = "Day A";
    dayA.exercises = {squat, bench};

    PlanRecord plan;
    plan.userId = userId;
    plan.name = "Test Plan";
    plan.daysPerWeek = 1;
    plan.isActive = true;
    plan.days = {dayA};

    const int64_t planId = repo.save(plan);
    ASSERT_GT(planId, 0);

    const auto loaded = repo.findById(planId);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->name, "Test Plan");
    ASSERT_EQ(loaded->days.size(), 1u);
    ASSERT_EQ(loaded->days[0].exercises.size(), 2u);
    EXPECT_EQ(loaded->days[0].exercises[0].position, 0);
    EXPECT_EQ(loaded->days[0].exercises[1].position, 1);
    ASSERT_TRUE(loaded->days[0].exercises[0].weightKg.has_value());
    EXPECT_DOUBLE_EQ(*loaded->days[0].exercises[0].weightKg, 100.0);
    EXPECT_FALSE(loaded->days[0].exercises[1].weightKg.has_value());
    EXPECT_TRUE(loaded->isActive);

    // UPDATE: replacing days.
    auto changed = *loaded;
    changed.name = "Updated Plan";
    changed.days.clear();
    EXPECT_EQ(repo.save(changed), planId);
    const auto updated = repo.findById(planId);
    EXPECT_EQ(updated->name, "Updated Plan");
    EXPECT_TRUE(updated->days.empty());

    // One active plan per user.
    PlanRecord second = plan;
    second.id = -1;
    second.name = "Second Plan";
    second.isActive = true;
    const int64_t secondId = repo.save(second);
    ASSERT_GT(secondId, 0);
    EXPECT_FALSE(repo.findById(planId)->isActive);
    EXPECT_TRUE(repo.findById(secondId)->isActive);

    EXPECT_EQ(repo.findByUser(userId).size(), 2u);
    EXPECT_TRUE(repo.remove(secondId));

    // Deleting a profile cascades to its plans.
    EXPECT_TRUE(UserRepository(*db).remove(userId));
    EXPECT_FALSE(repo.findById(planId).has_value());
}

TEST_F(DatabaseTest, RejectsUnknownExerciseForOneRepMax) {
    core::UserConstraints constraints;
    const auto profile = core::UserProfile::create(
        "FK Test", 25, core::Gender::Female, 165.0, 60.0,
        core::ExperienceLevel::Novice, core::TrainingGoal::Hypertrophy,
        core::TrainingFrequency::Days3, constraints, -1.0, 3);
    ASSERT_TRUE(profile.has_value());
    const auto withUnknown = profile->withEstimated1RM(9999, 50.0);

    EXPECT_THROW(UserRepository(*db).save(withUnknown), std::runtime_error);
}

TEST_F(DatabaseTest, TransactionGuardRejectsNestedCalls) {
    EXPECT_FALSE(db->inTransaction());

    db->beginTransaction();
    EXPECT_TRUE(db->inTransaction());
    EXPECT_THROW(db->beginTransaction(), std::logic_error);

    db->rollback();
    EXPECT_FALSE(db->inTransaction());
    EXPECT_THROW(db->commit(), std::logic_error);
    EXPECT_THROW(db->rollback(), std::logic_error);
}

TEST_F(DatabaseTest, FindAllLoadsOneRepMaxesForEveryProfile) {
    const int64_t firstId = createUser();
    ASSERT_GT(firstId, 0);

    core::UserConstraints constraints;
    const auto second = core::UserProfile::create(
        "Second User", 40, core::Gender::Female, 165.0, 60.0,
        core::ExperienceLevel::Novice, core::TrainingGoal::Hypertrophy,
        core::TrainingFrequency::Days2, constraints, -1.0, 6);
    ASSERT_TRUE(second.has_value());
    const int64_t secondId = UserRepository(*db).save(second->withEstimated1RM(2, 55.0));
    ASSERT_GT(secondId, 0);

    const auto all = UserRepository(*db).findAll();
    ASSERT_EQ(all.size(), 2u);

    EXPECT_EQ(all[0].getId(), firstId);
    EXPECT_DOUBLE_EQ(all[0].getEstimated1RM(1), 100.0);
    EXPECT_DOUBLE_EQ(all[0].getEstimated1RM(3), 80.0);
    EXPECT_DOUBLE_EQ(all[0].getEstimated1RM(2), 0.0);

    EXPECT_EQ(all[1].getId(), secondId);
    EXPECT_DOUBLE_EQ(all[1].getEstimated1RM(2), 55.0);
    EXPECT_DOUBLE_EQ(all[1].getEstimated1RM(1), 0.0);
}

TEST_F(DatabaseTest, FindCompatibleFiltersInSql) {
    ExerciseRepository repo(*db);

    const auto names = [&repo](const core::UserConstraints& constraints) {
        std::vector<std::string> result;
        for (const auto& exercise : repo.findCompatible(constraints)) {
            result.push_back(exercise.getName());
        }
        return result;
    };

    const auto contains = [](const std::vector<std::string>& values, const std::string& name) {
        return std::find(values.begin(), values.end(), name) != values.end();
    };

    core::UserConstraints barbellOnly;
    barbellOnly.availableEquipment = core::Equipment::Barbell;

    const auto withBarbell = names(barbellOnly);
    EXPECT_TRUE(contains(withBarbell, "Overhead Press"));
    EXPECT_FALSE(contains(withBarbell, "Pull-up"));
    EXPECT_FALSE(contains(withBarbell, "Bench Press"));

    core::UserConstraints avoidingOverhead = barbellOnly;
    avoidingOverhead.userAvoidOverheadMotion = true;
    EXPECT_FALSE(contains(names(avoidingOverhead), "Overhead Press"));

    core::UserConstraints kneePain = barbellOnly;
    kneePain.addConstraint(core::HealthConstraint::KneePain);
    EXPECT_FALSE(contains(names(kneePain), "Barbell Back Squat"));

    core::UserConstraints bodyweightOnly;
    const auto bodyweight = names(bodyweightOnly);
    EXPECT_TRUE(contains(bodyweight, "Push-up"));
    EXPECT_FALSE(contains(bodyweight, "Overhead Press"));
}

} // namespace
} // namespace workout::infra