#include "core/Exercise.h"
#include "core/PlanGenerator.h"
#include "core/User.h"
#include "core/WorkoutPlan.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace workout::core;

UserProfile makeUser(TrainingGoal goal,
                     TrainingFrequency frequency,
                     UserConstraints constraints = {}) {
    auto profile = UserProfile::create("Test",
                                       30,
                                       Gender::Male,
                                       180.0,
                                       80.0,
                                       ExperienceLevel::Intermediate,
                                       goal,
                                       frequency,
                                       constraints);
    EXPECT_TRUE(profile.has_value());
    return *profile;
}

Exercise makeExercise(ExerciseId id,
                      std::string name,
                      MuscleGroup muscle,
                      MovementPattern pattern,
                      ExerciseStructure structure,
                      bool unilateral = false,
                      int setDurationSec = 60,
                      Equipment requiredEquipment = Equipment::None,
                      bool axialLoading = false) {
    const PurposeType purpose = structure == ExerciseStructure::Compound
                                    ? PurposeType::Main
                                    : PurposeType::Accessory;
    auto exercise = Exercise::create(id,
                                     std::move(name),
                                     "",
                                     muscle,
                                     {},
                                     structure,
                                     ExerciseKind::Resistance,
                                     purpose,
                                     pattern,
                                     requiredEquipment,
                                     Equipment::None,
                                     3,
                                     5.0,
                                     setDurationSec,
                                     "",
                                     "",
                                     false,
                                     axialLoading,
                                     false,
                                     false,
                                     false,
                                     unilateral);
    EXPECT_TRUE(exercise.has_value());
    return *exercise;
}

std::vector<Exercise> makeCatalog() {
    return {
        makeExercise(1, "Squat", MuscleGroup::Legs_Quads,
                     MovementPattern::Squat, ExerciseStructure::Compound),
        makeExercise(2, "Leg Extension", MuscleGroup::Legs_Quads,
                     MovementPattern::Other, ExerciseStructure::Isolation),
        makeExercise(3, "Push-up", MuscleGroup::Chest_Upper,
                     MovementPattern::Push_Horizontal, ExerciseStructure::Compound),
        makeExercise(4, "Chest Fly", MuscleGroup::Chest_Upper,
                     MovementPattern::Other, ExerciseStructure::Isolation),
        makeExercise(5, "Pull-up", MuscleGroup::Back_Lats,
                     MovementPattern::Pull_Vertical, ExerciseStructure::Compound),
        makeExercise(6, "Row", MuscleGroup::Back_Lats,
                     MovementPattern::Pull_Horizontal, ExerciseStructure::Compound),
        makeExercise(7, "Shoulder Press", MuscleGroup::Shoulders_Front,
                     MovementPattern::Push_Vertical, ExerciseStructure::Compound),
        makeExercise(8, "Lateral Raise", MuscleGroup::Shoulders_Side,
                     MovementPattern::Other, ExerciseStructure::Isolation),
        makeExercise(9, "Plank", MuscleGroup::Core_Abs,
                     MovementPattern::Other, ExerciseStructure::Isolation),
        makeExercise(10, "Crunch", MuscleGroup::Core_Obliques,
                     MovementPattern::Other, ExerciseStructure::Isolation),
        makeExercise(11, "Curl", MuscleGroup::Arms_Biceps,
                     MovementPattern::Other, ExerciseStructure::Isolation),
        makeExercise(12, "Triceps Extension", MuscleGroup::Arms_Triceps,
                     MovementPattern::Other, ExerciseStructure::Isolation)
    };
}

bool containsExercise(const WorkoutPlan& plan, ExerciseId exerciseId) {
    for (const PlanDay& day : plan.getDays()) {
        for (const PlannedExercise& exercise : day.exercises) {
            if (exercise.exerciseId == exerciseId) {
                return true;
            }
        }
    }
    return false;
}

int dayDurationSeconds(const PlanDay& day,
                       const std::vector<Exercise>& catalog) {
    int total = 0;
    for (const PlannedExercise& planned : day.exercises) {
        const auto exercise = std::find_if(
            catalog.begin(), catalog.end(),
            [exerciseId = planned.exerciseId](const Exercise& candidate) {
                return candidate.getId() == exerciseId;
            });
        if (exercise == catalog.end()) {
            return -1;
        }
        total += planned.sets *
                 (exercise->getEstimatedSetDurationSec() + planned.restSec);
    }
    return total;
}

} // namespace

TEST(PlanGenerator, EmptyCatalogReturnsEmptyPlan) {
    const UserProfile user =
        makeUser(TrainingGoal::Hypertrophy, TrainingFrequency::Days3);

    const WorkoutPlan plan = PlanGenerator::generate(user, {});

    EXPECT_TRUE(plan.isEmpty());
    EXPECT_EQ(plan.getDayCount(), 0u);
}

TEST(PlanGenerator, IncompatibleCatalogReturnsEmptyPlan) {
    const UserProfile user =
        makeUser(TrainingGoal::Hypertrophy, TrainingFrequency::Days2);

    const std::vector<Exercise> catalog{
        makeExercise(1, "Barbell Bench", MuscleGroup::Chest_Upper,
                     MovementPattern::Push_Horizontal,
                     ExerciseStructure::Compound, false, 60,
                     Equipment::Barbell)
    };

    const WorkoutPlan plan = PlanGenerator::generate(user, catalog);

    EXPECT_TRUE(plan.isEmpty());
}

TEST(PlanGenerator, UsesFrequencyForDayCount) {
    const std::vector<Exercise> catalog = makeCatalog();

    const std::array<TrainingFrequency, 5> frequencies{
        TrainingFrequency::Days2,
        TrainingFrequency::Days3,
        TrainingFrequency::Days4,
        TrainingFrequency::Days5,
        TrainingFrequency::Days6
    };

    for (const TrainingFrequency frequency : frequencies) {
        const UserProfile user =
            makeUser(TrainingGoal::Hypertrophy, frequency);
        const WorkoutPlan plan = PlanGenerator::generate(user, catalog);

        const std::size_t expectedDays =
            static_cast<std::size_t>(static_cast<int>(frequency));
        ASSERT_EQ(plan.getDayCount(), expectedDays)
            << "frequency=" << static_cast<int>(frequency);

        for (std::size_t index = 0; index < plan.getDayCount(); ++index) {
            const PlanDay& day = plan.getDays()[index];
            EXPECT_EQ(day.dayIndex, static_cast<int>(index) + 1);
            EXPECT_FALSE(day.name.empty());
            EXPECT_FALSE(day.exercises.empty());
        }
    }
}

TEST(PlanGenerator, FiltersIncompatibleExercises) {
    UserConstraints constraints;
    constraints.availableEquipment = Equipment::None;
    constraints.userAvoidAxialLoading = true;

    const UserProfile user =
        makeUser(TrainingGoal::Hypertrophy, TrainingFrequency::Days2,
                 constraints);

    const std::vector<Exercise> catalog{
        makeExercise(1, "Push-up", MuscleGroup::Chest_Upper,
                     MovementPattern::Push_Horizontal,
                     ExerciseStructure::Compound),
        makeExercise(2, "Axial Squat", MuscleGroup::Legs_Quads,
                     MovementPattern::Squat, ExerciseStructure::Compound,
                     false, 60, Equipment::None, true),
        makeExercise(3, "Barbell Bench", MuscleGroup::Chest_Upper,
                     MovementPattern::Push_Horizontal,
                     ExerciseStructure::Compound, false, 60,
                     Equipment::Barbell)
    };

    const WorkoutPlan plan = PlanGenerator::generate(user, catalog);

    ASSERT_FALSE(plan.isEmpty());
    EXPECT_TRUE(containsExercise(plan, 1));
    EXPECT_FALSE(containsExercise(plan, 2));
    EXPECT_FALSE(containsExercise(plan, 3));

    for (const PlanDay& day : plan.getDays()) {
        for (const PlannedExercise& planned : day.exercises) {
            const auto exercise = std::find_if(
                catalog.begin(), catalog.end(),
                [exerciseId = planned.exerciseId](const Exercise& candidate) {
                    return candidate.getId() == exerciseId;
                });
            ASSERT_NE(exercise, catalog.end());
            EXPECT_TRUE(exercise->isCompatibleWith(constraints));
        }
    }
}

TEST(PlanGenerator, RespectsSessionLimit) {
    UserConstraints constraints;
    constraints.maxSessionMinutes = 60;

    const UserProfile user =
        makeUser(TrainingGoal::Strength, TrainingFrequency::Days2,
                 constraints);

    const std::vector<Exercise> catalog{
        makeExercise(1, "Leg Press", MuscleGroup::Legs_Quads,
                     MovementPattern::Squat, ExerciseStructure::Compound,
                     false, 300),
        makeExercise(2, "Leg Extension", MuscleGroup::Legs_Quads,
                     MovementPattern::Other, ExerciseStructure::Isolation,
                     false, 300),
        makeExercise(3, "Leg Curl", MuscleGroup::Legs_Hamstrings,
                     MovementPattern::Other, ExerciseStructure::Isolation,
                     false, 300)
    };

    const WorkoutPlan plan = PlanGenerator::generate(user, catalog);

    ASSERT_EQ(plan.getDayCount(), 2u);
    for (const PlanDay& day : plan.getDays()) {
        EXPECT_EQ(day.exercises.size(), 1u);
        const int duration = dayDurationSeconds(day, catalog);
        ASSERT_GE(duration, 0);
        EXPECT_LE(duration, 60 * 60);
    }
}

TEST(PlanGenerator, UsesDefaultSessionLimit) {
    const UserProfile user =
        makeUser(TrainingGoal::Strength, TrainingFrequency::Days2);

    const std::vector<Exercise> catalog{
        makeExercise(1, "Leg Press", MuscleGroup::Legs_Quads,
                     MovementPattern::Squat, ExerciseStructure::Compound,
                     false, 300),
        makeExercise(2, "Leg Extension", MuscleGroup::Legs_Quads,
                     MovementPattern::Other, ExerciseStructure::Isolation,
                     false, 300),
        makeExercise(3, "Leg Curl", MuscleGroup::Legs_Hamstrings,
                     MovementPattern::Other, ExerciseStructure::Isolation,
                     false, 300)
    };

    const WorkoutPlan plan = PlanGenerator::generate(user, catalog);

    ASSERT_EQ(plan.getDayCount(), 2u);
    for (const PlanDay& day : plan.getDays()) {
        EXPECT_EQ(day.exercises.size(), 2u);
        const int duration = dayDurationSeconds(day, catalog);
        ASSERT_GE(duration, 0);
        EXPECT_LE(duration, 90 * 60);
    }
}

TEST(PlanGenerator, AppliesGoalPrescriptions) {
    struct ExpectedPrescription {
        TrainingGoal goal;
        int compoundSets;
        int compoundReps;
        int compoundRestSec;
        int isolationSets;
        int isolationReps;
        int isolationRestSec;
    };

    const std::array<ExpectedPrescription, 4> expected{{
        {TrainingGoal::Strength, 4, 5, 270, 4, 5, 270},
        {TrainingGoal::Hypertrophy, 4, 10, 135, 3, 10, 135},
        {TrainingGoal::WeightLoss, 3, 12, 90, 3, 12, 90},
        {TrainingGoal::Maintenance, 3, 10, 135, 3, 10, 135}
    }};

    const std::vector<Exercise> catalog{
        makeExercise(1, "Push-up", MuscleGroup::Chest_Upper,
                     MovementPattern::Push_Horizontal,
                     ExerciseStructure::Compound),
        makeExercise(2, "Chest Fly", MuscleGroup::Chest_Upper,
                     MovementPattern::Other, ExerciseStructure::Isolation)
    };

    for (const ExpectedPrescription& entry : expected) {
        const UserProfile user =
            makeUser(entry.goal, TrainingFrequency::Days2);
        const WorkoutPlan plan = PlanGenerator::generate(user, catalog);

        ASSERT_EQ(plan.getDayCount(), 2u);
        for (const PlanDay& day : plan.getDays()) {
            ASSERT_EQ(day.exercises.size(), 2u);
            EXPECT_EQ(day.exercises[0].exerciseId, 1);
            EXPECT_EQ(day.exercises[1].exerciseId, 2);

            EXPECT_EQ(day.exercises[0].sets, entry.compoundSets);
            EXPECT_EQ(day.exercises[0].reps, entry.compoundReps);
            EXPECT_EQ(day.exercises[0].restSec, entry.compoundRestSec);

            EXPECT_EQ(day.exercises[1].sets, entry.isolationSets);
            EXPECT_EQ(day.exercises[1].reps, entry.isolationReps);
            EXPECT_EQ(day.exercises[1].restSec, entry.isolationRestSec);
        }
    }
}

TEST(PlanGenerator, PrefersUnilateralExercises) {
    UserConstraints constraints;
    constraints.userPreferUnilateral = true;

    const UserProfile user =
        makeUser(TrainingGoal::Hypertrophy, TrainingFrequency::Days2,
                 constraints);

    const std::vector<Exercise> catalog{
        makeExercise(1, "Bilateral Press", MuscleGroup::Chest_Upper,
                     MovementPattern::Push_Horizontal,
                     ExerciseStructure::Compound),
        makeExercise(2, "Unilateral Press", MuscleGroup::Chest_Upper,
                     MovementPattern::Push_Horizontal,
                     ExerciseStructure::Compound, true)
    };

    const WorkoutPlan plan = PlanGenerator::generate(user, catalog);

    ASSERT_EQ(plan.getDayCount(), 2u);
    for (const PlanDay& day : plan.getDays()) {
        ASSERT_EQ(day.exercises.size(), 2u);
        EXPECT_EQ(day.exercises[0].exerciseId, 2);
        EXPECT_EQ(day.exercises[1].exerciseId, 1);
    }
}

TEST(PlanGenerator, IgnoresNonPositiveExerciseIds) {
    const UserProfile user =
        makeUser(TrainingGoal::Hypertrophy, TrainingFrequency::Days2);

    const std::vector<Exercise> catalog{
        makeExercise(-1, "Unsaved", MuscleGroup::Chest_Upper,
                     MovementPattern::Push_Horizontal,
                     ExerciseStructure::Compound),
        makeExercise(0, "Zero", MuscleGroup::Chest_Upper,
                     MovementPattern::Push_Horizontal,
                     ExerciseStructure::Compound),
        makeExercise(1, "Saved", MuscleGroup::Chest_Upper,
                     MovementPattern::Push_Horizontal,
                     ExerciseStructure::Compound)
    };

    const WorkoutPlan plan = PlanGenerator::generate(user, catalog);

    ASSERT_FALSE(plan.isEmpty());
    for (const PlanDay& day : plan.getDays()) {
        for (const PlannedExercise& exercise : day.exercises) {
            EXPECT_EQ(exercise.exerciseId, 1);
        }
    }
}

TEST(PlanGenerator, IsDeterministic) {
    const UserProfile user =
        makeUser(TrainingGoal::Hypertrophy, TrainingFrequency::Days5);
    const std::vector<Exercise> catalog = makeCatalog();

    const WorkoutPlan first = PlanGenerator::generate(user, catalog);
    const WorkoutPlan second = PlanGenerator::generate(user, catalog);

    EXPECT_EQ(first.toJson(), second.toJson());
}
