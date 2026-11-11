#include "core/Exercise.h"
#include "core/User.h"

#include "TestHelpers.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <limits>
#include <string>

namespace {

using namespace workout::core;

namespace test = workout::test;

} // namespace

TEST(ExerciseMuscles, ToStringAndMajorGroup) {
    struct ExpectedMuscle {
        MuscleGroup muscle;
        const char* name;
        MajorMuscleGroup major;
    };

    const std::array<ExpectedMuscle, 18> expected{{
        {MuscleGroup::Chest_Upper, "Chest_Upper", MajorMuscleGroup::Chest},
        {MuscleGroup::Chest_Lower, "Chest_Lower", MajorMuscleGroup::Chest},
        {MuscleGroup::Back_Lats, "Back_Lats", MajorMuscleGroup::Back},
        {MuscleGroup::Back_Traps, "Back_Traps", MajorMuscleGroup::Back},
        {MuscleGroup::Back_Rhomboids, "Back_Rhomboids", MajorMuscleGroup::Back},
        {MuscleGroup::Legs_Quads, "Legs_Quads", MajorMuscleGroup::Legs},
        {MuscleGroup::Legs_Glutes, "Legs_Glutes", MajorMuscleGroup::Legs},
        {MuscleGroup::Legs_Hamstrings, "Legs_Hamstrings", MajorMuscleGroup::Legs},
        {MuscleGroup::Legs_Calves, "Legs_Calves", MajorMuscleGroup::Legs},
        {MuscleGroup::Shoulders_Front, "Shoulders_Front", MajorMuscleGroup::Shoulders},
        {MuscleGroup::Shoulders_Side, "Shoulders_Side", MajorMuscleGroup::Shoulders},
        {MuscleGroup::Shoulders_Rear, "Shoulders_Rear", MajorMuscleGroup::Shoulders},
        {MuscleGroup::Arms_Biceps, "Arms_Biceps", MajorMuscleGroup::Arms},
        {MuscleGroup::Arms_Triceps, "Arms_Triceps", MajorMuscleGroup::Arms},
        {MuscleGroup::Arms_Forearms, "Arms_Forearms", MajorMuscleGroup::Arms},
        {MuscleGroup::Core_Abs, "Core_Abs", MajorMuscleGroup::Core},
        {MuscleGroup::Core_Obliques, "Core_Obliques", MajorMuscleGroup::Core},
        {MuscleGroup::Core_LowerBack, "Core_LowerBack", MajorMuscleGroup::Core}
    }};

    for (const auto& entry : expected) {
        EXPECT_STREQ(toString(entry.muscle), entry.name);
        EXPECT_EQ(toMajorGroup(entry.muscle), entry.major) << entry.name;
    }

    EXPECT_STREQ(toString(MuscleGroup::Count), "Unknown");
    EXPECT_EQ(toMajorGroup(MuscleGroup::Count), MajorMuscleGroup::Unknown);
    EXPECT_STREQ(toString(static_cast<MuscleGroup>(200)), "Unknown");
    EXPECT_EQ(toMajorGroup(static_cast<MuscleGroup>(200)), MajorMuscleGroup::Unknown);
}

TEST(ExerciseCreate, CreatesValidExercise) {
    const auto exercise = test::makeValidExercise(7);
    ASSERT_TRUE(exercise.has_value());

    EXPECT_EQ(exercise->getId(), 7);
    EXPECT_EQ(exercise->getName(), "Barbell Bench Press");
    EXPECT_EQ(exercise->getDescription(), "Horizontal press");
    EXPECT_EQ(exercise->getPrimaryMuscle(), MuscleGroup::Chest_Upper);
    ASSERT_EQ(exercise->getSecondaryMuscles().size(), 2u);
    EXPECT_EQ(exercise->getSecondaryMuscles()[0], MuscleGroup::Arms_Triceps);
    EXPECT_EQ(exercise->getSecondaryMuscles()[1], MuscleGroup::Shoulders_Front);
    EXPECT_EQ(exercise->getStructure(), ExerciseStructure::Compound);
    EXPECT_EQ(exercise->getKind(), ExerciseKind::Resistance);
    EXPECT_EQ(exercise->getPurposeType(), PurposeType::Main);
    EXPECT_EQ(exercise->getMovementPattern(), MovementPattern::Push_Horizontal);
    EXPECT_EQ(exercise->getRequiredEquipment(),
              Equipment::Barbell | Equipment::Bench);
    EXPECT_EQ(exercise->getOptionalEquipment(), Equipment::None);
    EXPECT_EQ(exercise->getDifficulty(), 3);
    EXPECT_DOUBLE_EQ(exercise->getMET(), 6.0);
    EXPECT_EQ(exercise->getEstimatedSetDurationSec(), 90);
    EXPECT_EQ(exercise->getInstructions(), "Lie on the bench and press.");
    EXPECT_EQ(exercise->getVideoUrl(), "https://example.com/bench");
    EXPECT_TRUE(exercise->isPowerliftingLift());
    EXPECT_FALSE(exercise->hasAxialLoading());
    EXPECT_FALSE(exercise->hasSpinalFlexion());
    EXPECT_FALSE(exercise->hasOverheadMotion());
    EXPECT_FALSE(exercise->hasDeepKneeFlexion());
    EXPECT_FALSE(exercise->isUnilateral());
    EXPECT_FALSE(exercise->getRegressionId().has_value());
    EXPECT_FALSE(exercise->getProgressionId().has_value());
}

TEST(ExerciseCreate, RejectsInvalidData) {
    const auto expectRejected = [](auto mutate) {
        auto data = test::validExerciseData();
        mutate(data);
        EXPECT_FALSE(test::createExercise(data).has_value());
    };

    expectRejected([](ExerciseData& data) { data.id = -2; });
    expectRejected([](ExerciseData& data) { data.name = ""; });
    expectRejected([](ExerciseData& data) { data.name = "   "; });
    expectRejected([](ExerciseData& data) { data.name = std::string(101, 'x'); });
    expectRejected([](ExerciseData& data) { data.description = std::string(2001, 'x'); });
    expectRejected([](ExerciseData& data) { data.instructions = std::string(10001, 'x'); });
    expectRejected([](ExerciseData& data) { data.videoUrl = std::string(501, 'x'); });

    expectRejected([](ExerciseData& data) {
        data.primaryMuscle = static_cast<MuscleGroup>(200);
    });
    expectRejected([](ExerciseData& data) {
        data.secondaryMuscles = {static_cast<MuscleGroup>(200)};
    });
    expectRejected([](ExerciseData& data) {
        data.secondaryMuscles = {MuscleGroup::Chest_Upper};
    });
    expectRejected([](ExerciseData& data) {
        data.secondaryMuscles = {MuscleGroup::Arms_Biceps, MuscleGroup::Arms_Biceps};
    });

    expectRejected([](ExerciseData& data) {
        data.structure = static_cast<ExerciseStructure>(9);
    });
    expectRejected([](ExerciseData& data) {
        data.kind = static_cast<ExerciseKind>(9);
    });
    expectRejected([](ExerciseData& data) {
        data.purposeType = static_cast<PurposeType>(9);
    });
    expectRejected([](ExerciseData& data) {
        data.pattern = static_cast<MovementPattern>(10);
    });

    expectRejected([](ExerciseData& data) {
        data.requiredEquipment = static_cast<Equipment>(1u << 15);
    });
    expectRejected([](ExerciseData& data) {
        data.optionalEquipment = static_cast<Equipment>(1u << 15);
    });
    expectRejected([](ExerciseData& data) {
        data.optionalEquipment = Equipment::Barbell;
    });

    expectRejected([](ExerciseData& data) { data.difficulty = 0; });
    expectRejected([](ExerciseData& data) { data.difficulty = 6; });
    expectRejected([](ExerciseData& data) { data.met = -1.0; });
    expectRejected([](ExerciseData& data) { data.met = 31.0; });
    expectRejected([](ExerciseData& data) {
        data.met = std::numeric_limits<double>::infinity();
    });
    expectRejected([](ExerciseData& data) {
        data.met = std::numeric_limits<double>::quiet_NaN();
    });

    expectRejected([](ExerciseData& data) { data.estimatedSetDurationSec = 4; });
    expectRejected([](ExerciseData& data) { data.estimatedSetDurationSec = 1801; });

    expectRejected([](ExerciseData& data) { data.regressionId = 0; });
    expectRejected([](ExerciseData& data) { data.progressionId = 0; });
    expectRejected([](ExerciseData& data) {
        data.id = 5;
        data.regressionId = 5;
    });
    expectRejected([](ExerciseData& data) {
        data.id = 5;
        data.progressionId = 5;
    });
    expectRejected([](ExerciseData& data) {
        data.regressionId = 7;
        data.progressionId = 7;
    });
}

TEST(ExerciseCreate, AcceptsBoundaryValues) {
    auto data = test::validExerciseData();
    data.difficulty = 1;
    data.met = 0.0;
    data.estimatedSetDurationSec = 5;
    EXPECT_TRUE(test::createExercise(data).has_value());

    data.difficulty = 5;
    data.met = 30.0;
    data.estimatedSetDurationSec = 1800;
    EXPECT_TRUE(test::createExercise(data).has_value());

    data.id = 0;
    EXPECT_TRUE(test::createExercise(data).has_value());
}

TEST(ExerciseCreateFromDb, RequiresNonNegativeId) {
    auto data = test::validExerciseData(0);
    const auto exercise = Exercise::createFromDb(data);
    EXPECT_EQ(exercise.getId(), 0);

    data.id = -1;
    EXPECT_THROW((void)Exercise::createFromDb(data), std::invalid_argument);
}

TEST(ExerciseCreateFromDb, DoesNotValidateOtherFields) {
    auto data = test::validExerciseData(1);
    data.difficulty = 99;

    const auto exercise = Exercise::createFromDb(data);
    EXPECT_EQ(exercise.getDifficulty(), 99);
}

TEST(ExerciseCompatibility, RequiresEquipment) {
    const auto bench = test::makeValidExercise(1);
    ASSERT_TRUE(bench.has_value());

    UserConstraints none;
    EXPECT_FALSE(bench->isCompatibleWith(none));

    UserConstraints partial;
    partial.availableEquipment = Equipment::Barbell;
    EXPECT_FALSE(bench->isCompatibleWith(partial));

    UserConstraints full;
    full.availableEquipment = Equipment::Barbell | Equipment::Bench;
    EXPECT_TRUE(bench->isCompatibleWith(full));
}

TEST(ExerciseCompatibility, RespectsAxialLoading) {
    auto data = test::validExerciseData(1);
    data.axialLoading = true;
    const auto exercise = Exercise::createFromDb(data);

    UserConstraints constraints;
    constraints.availableEquipment = data.requiredEquipment;
    EXPECT_TRUE(exercise.isCompatibleWith(constraints));

    constraints.userAvoidAxialLoading = true;
    EXPECT_FALSE(exercise.isCompatibleWith(constraints));

    UserConstraints lowerBack;
    lowerBack.availableEquipment = data.requiredEquipment;
    lowerBack.addConstraint(HealthConstraint::LowerBackPain);
    EXPECT_FALSE(exercise.isCompatibleWith(lowerBack));

    UserConstraints hernia;
    hernia.availableEquipment = data.requiredEquipment;
    hernia.addConstraint(HealthConstraint::Hernia);
    EXPECT_FALSE(exercise.isCompatibleWith(hernia));
}

TEST(ExerciseCompatibility, RespectsSpinalFlexion) {
    auto data = test::validExerciseData(1);
    data.spinalFlexion = true;
    const auto exercise = Exercise::createFromDb(data);

    UserConstraints constraints;
    constraints.availableEquipment = data.requiredEquipment;
    EXPECT_TRUE(exercise.isCompatibleWith(constraints));

    constraints.userAvoidSpinalFlexion = true;
    EXPECT_FALSE(exercise.isCompatibleWith(constraints));

    UserConstraints lowerBack;
    lowerBack.availableEquipment = data.requiredEquipment;
    lowerBack.addConstraint(HealthConstraint::LowerBackPain);
    EXPECT_FALSE(exercise.isCompatibleWith(lowerBack));
}

TEST(ExerciseCompatibility, RespectsOverheadMotion) {
    auto data = test::validExerciseData(1);
    data.overheadMotion = true;
    const auto exercise = Exercise::createFromDb(data);

    UserConstraints constraints;
    constraints.availableEquipment = data.requiredEquipment;
    EXPECT_TRUE(exercise.isCompatibleWith(constraints));

    constraints.userAvoidOverheadMotion = true;
    EXPECT_FALSE(exercise.isCompatibleWith(constraints));

    UserConstraints impingement;
    impingement.availableEquipment = data.requiredEquipment;
    impingement.addConstraint(HealthConstraint::ShoulderImpingement);
    EXPECT_FALSE(exercise.isCompatibleWith(impingement));
}

TEST(ExerciseCompatibility, RespectsDeepKneeFlexion) {
    auto data = test::validExerciseData(1);
    data.deepKneeFlexion = true;
    const auto exercise = Exercise::createFromDb(data);

    UserConstraints constraints;
    constraints.availableEquipment = data.requiredEquipment;
    EXPECT_TRUE(exercise.isCompatibleWith(constraints));

    constraints.userAvoidDeepKneeFlexion = true;
    EXPECT_FALSE(exercise.isCompatibleWith(constraints));

    UserConstraints kneePain;
    kneePain.availableEquipment = data.requiredEquipment;
    kneePain.addConstraint(HealthConstraint::KneePain);
    EXPECT_FALSE(exercise.isCompatibleWith(kneePain));

    UserConstraints hipImpingement;
    hipImpingement.availableEquipment = data.requiredEquipment;
    hipImpingement.addConstraint(HealthConstraint::HipImpingement);
    EXPECT_FALSE(exercise.isCompatibleWith(hipImpingement));
}

TEST(ExerciseCompatibility, IgnoresUnrelatedConstraints) {
    auto data = test::validExerciseData(1);
    data.axialLoading = true;
    const auto exercise = Exercise::createFromDb(data);

    UserConstraints constraints;
    constraints.availableEquipment = data.requiredEquipment;
    constraints.addConstraint(HealthConstraint::ElbowTendinopathy);
    constraints.addConstraint(HealthConstraint::WristPain);

    EXPECT_TRUE(exercise.isCompatibleWith(constraints));
}

TEST(ExercisePenalty, PrefersUnilateralExercises) {
    const auto bilateral = test::makeValidExercise(1);
    ASSERT_TRUE(bilateral.has_value());

    UserConstraints neutral;
    EXPECT_EQ(bilateral->unilateralPenalty(neutral), 0);

    UserConstraints prefer;
    prefer.userPreferUnilateral = true;
    EXPECT_EQ(bilateral->unilateralPenalty(prefer), 10);

    auto data = test::validExerciseData(2);
    data.isUnilateral = true;
    const auto unilateral = Exercise::createFromDb(data);
    EXPECT_EQ(unilateral.unilateralPenalty(prefer), 0);
}
