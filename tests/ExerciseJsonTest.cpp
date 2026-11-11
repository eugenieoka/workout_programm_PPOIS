#include "core/Exercise.h"

#include "TestHelpers.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace {

using namespace workout::core;

namespace test = workout::test;

} // namespace

TEST(ExerciseJson, SerializesAllFields) {
    const auto exercise = test::makeValidExercise(7);
    ASSERT_TRUE(exercise.has_value());

    const std::string text = exercise->toJson();

    EXPECT_NE(text.find("\"Id\": 7"), std::string::npos);
    EXPECT_NE(text.find("\"Name\": \"Barbell Bench Press\""), std::string::npos);
    EXPECT_NE(text.find("\"PrimaryMuscle\": \"Chest_Upper\""), std::string::npos);
    EXPECT_NE(text.find("\"Arms_Triceps\""), std::string::npos);
    EXPECT_NE(text.find("\"Shoulders_Front\""), std::string::npos);
    EXPECT_NE(text.find("\"Structure\": \"Compound\""), std::string::npos);
    EXPECT_NE(text.find("\"Kind\": \"Resistance\""), std::string::npos);
    EXPECT_NE(text.find("\"PurposeType\": \"Main\""), std::string::npos);
    EXPECT_NE(text.find("\"MovementPattern\": \"Push_Horizontal\""),
              std::string::npos);
    EXPECT_NE(text.find("\"Barbell\""), std::string::npos);
    EXPECT_NE(text.find("\"Bench\""), std::string::npos);
    EXPECT_NE(text.find("\"Difficulty\": 3"), std::string::npos);
    EXPECT_NE(text.find("\"Met\": 6.0"), std::string::npos);
    EXPECT_NE(text.find("\"EstimatedSetDurationSec\": 90"), std::string::npos);
    EXPECT_NE(text.find("\"IsPowerliftingLift\": true"), std::string::npos);
    EXPECT_NE(text.find("\"Flags\": {"), std::string::npos);
    EXPECT_NE(text.find("\"AxialLoading\": false"), std::string::npos);
    EXPECT_NE(text.find("\"RegressionId\": null"), std::string::npos);
    EXPECT_NE(text.find("\"ProgressionId\": null"), std::string::npos);
}

TEST(ExerciseJson, RoundTrips) {
    const auto exercise = test::makeValidExercise(7);
    ASSERT_TRUE(exercise.has_value());

    const std::string text = exercise->toJson();
    const Exercise restored = Exercise::fromJson(text);

    EXPECT_EQ(restored.getId(), exercise->getId());
    EXPECT_EQ(restored.getName(), exercise->getName());
    EXPECT_EQ(restored.getDescription(), exercise->getDescription());
    EXPECT_EQ(restored.getPrimaryMuscle(), exercise->getPrimaryMuscle());
    EXPECT_EQ(restored.getSecondaryMuscles(), exercise->getSecondaryMuscles());
    EXPECT_EQ(restored.getStructure(), exercise->getStructure());
    EXPECT_EQ(restored.getKind(), exercise->getKind());
    EXPECT_EQ(restored.getPurposeType(), exercise->getPurposeType());
    EXPECT_EQ(restored.getMovementPattern(), exercise->getMovementPattern());
    EXPECT_EQ(restored.getRequiredEquipment(), exercise->getRequiredEquipment());
    EXPECT_EQ(restored.getOptionalEquipment(), exercise->getOptionalEquipment());
    EXPECT_EQ(restored.getDifficulty(), exercise->getDifficulty());
    EXPECT_DOUBLE_EQ(restored.getMET(), exercise->getMET());
    EXPECT_EQ(restored.getEstimatedSetDurationSec(),
              exercise->getEstimatedSetDurationSec());
    EXPECT_EQ(restored.getInstructions(), exercise->getInstructions());
    EXPECT_EQ(restored.getVideoUrl(), exercise->getVideoUrl());
    EXPECT_EQ(restored.isPowerliftingLift(), exercise->isPowerliftingLift());

    EXPECT_EQ(restored.toJson(), text);
}

TEST(ExerciseJson, RoundTripsFlagsAndSubstitutionChain) {
    auto data = test::validExerciseData(3);
    data.axialLoading = true;
    data.spinalFlexion = true;
    data.overheadMotion = true;
    data.deepKneeFlexion = true;
    data.isUnilateral = true;
    data.regressionId = 1;
    data.progressionId = 2;

    const auto exercise = test::createExercise(data);
    ASSERT_TRUE(exercise.has_value());

    const std::string text = exercise->toJson();
    const Exercise restored = Exercise::fromJson(text);

    EXPECT_TRUE(restored.hasAxialLoading());
    EXPECT_TRUE(restored.hasSpinalFlexion());
    EXPECT_TRUE(restored.hasOverheadMotion());
    EXPECT_TRUE(restored.hasDeepKneeFlexion());
    EXPECT_TRUE(restored.isUnilateral());
    ASSERT_TRUE(restored.getRegressionId().has_value());
    ASSERT_TRUE(restored.getProgressionId().has_value());
    EXPECT_EQ(*restored.getRegressionId(), 1);
    EXPECT_EQ(*restored.getProgressionId(), 2);
    EXPECT_EQ(restored.toJson(), text);
}

TEST(ExerciseJson, AppliesDefaultsForMinimalJson) {
    const Exercise exercise = Exercise::fromJson(
        R"({"Name":"Push-up","PrimaryMuscle":"Chest_Upper"})");

    EXPECT_EQ(exercise.getId(), -1);
    EXPECT_EQ(exercise.getName(), "Push-up");
    EXPECT_EQ(exercise.getDescription(), "");
    EXPECT_EQ(exercise.getPrimaryMuscle(), MuscleGroup::Chest_Upper);
    EXPECT_TRUE(exercise.getSecondaryMuscles().empty());
    EXPECT_EQ(exercise.getStructure(), ExerciseStructure::Compound);
    EXPECT_EQ(exercise.getKind(), ExerciseKind::Resistance);
    EXPECT_EQ(exercise.getPurposeType(), PurposeType::Main);
    EXPECT_EQ(exercise.getMovementPattern(), MovementPattern::Other);
    EXPECT_EQ(exercise.getRequiredEquipment(), Equipment::None);
    EXPECT_EQ(exercise.getOptionalEquipment(), Equipment::None);
    EXPECT_EQ(exercise.getDifficulty(), 3);
    EXPECT_DOUBLE_EQ(exercise.getMET(), 0.0);
    EXPECT_EQ(exercise.getEstimatedSetDurationSec(), 60);
    EXPECT_EQ(exercise.getInstructions(), "");
    EXPECT_EQ(exercise.getVideoUrl(), "");
    EXPECT_FALSE(exercise.isPowerliftingLift());
    EXPECT_FALSE(exercise.hasAxialLoading());
    EXPECT_FALSE(exercise.hasSpinalFlexion());
    EXPECT_FALSE(exercise.hasOverheadMotion());
    EXPECT_FALSE(exercise.hasDeepKneeFlexion());
    EXPECT_FALSE(exercise.isUnilateral());
    EXPECT_FALSE(exercise.getRegressionId().has_value());
    EXPECT_FALSE(exercise.getProgressionId().has_value());
}

TEST(ExerciseJson, ReadsFlagsAndSubstitutionChain) {
    const Exercise exercise = Exercise::fromJson(R"({
        "Id": 1,
        "Name": "Split Squat",
        "PrimaryMuscle": "Legs_Quads",
        "Flags": {
            "AxialLoading": true,
            "SpinalFlexion": true,
            "OverheadMotion": true,
            "DeepKneeFlexion": true,
            "IsUnilateral": true
        },
        "RegressionId": 5,
        "ProgressionId": 6
    })");

    EXPECT_EQ(exercise.getId(), 1);
    EXPECT_TRUE(exercise.hasAxialLoading());
    EXPECT_TRUE(exercise.hasSpinalFlexion());
    EXPECT_TRUE(exercise.hasOverheadMotion());
    EXPECT_TRUE(exercise.hasDeepKneeFlexion());
    EXPECT_TRUE(exercise.isUnilateral());
    ASSERT_TRUE(exercise.getRegressionId().has_value());
    ASSERT_TRUE(exercise.getProgressionId().has_value());
    EXPECT_EQ(*exercise.getRegressionId(), 5);
    EXPECT_EQ(*exercise.getProgressionId(), 6);
}

TEST(ExerciseJson, RejectsMalformedJson) {
    EXPECT_THROW(Exercise::fromJson("{"), std::runtime_error);
    EXPECT_THROW(Exercise::fromJson("[]"), std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(R"({"Name":"Push-up"})"), std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(R"({"PrimaryMuscle":"Chest_Upper"})"),
                 std::runtime_error);
}

TEST(ExerciseJson, RejectsUnknownEnumValues) {
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Nope"})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","Structure":"Nope"})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","Kind":"Nope"})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","PurposeType":"Nope"})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","MovementPattern":"Nope"})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","SecondaryMuscles":["Nope"]})"),
                 std::runtime_error);
}

TEST(ExerciseJson, RejectsInvalidEquipment) {
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","RequiredEquipment":["Nope"]})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","RequiredEquipment":"Barbell"})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","RequiredEquipment":["Barbell"],"OptionalEquipment":["Barbell"]})"),
                 std::runtime_error);
}

TEST(ExerciseJson, RejectsInvalidFlags) {
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","Flags":[]})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","Flags":{"AxialLoading":"yes"}})"),
                 std::runtime_error);
}

TEST(ExerciseJson, RejectsInvalidFields) {
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"","PrimaryMuscle":"Chest_Upper"})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","Difficulty":9})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","Met":31.0})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","EstimatedSetDurationSec":4})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","Id":-2})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","SecondaryMuscles":["Chest_Upper"]})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","SecondaryMuscles":["Arms_Biceps","Arms_Biceps"]})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","RegressionId":0})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","Id":5,"RegressionId":5})"),
                 std::runtime_error);
    EXPECT_THROW(Exercise::fromJson(
                     R"({"Name":"X","PrimaryMuscle":"Chest_Upper","RegressionId":7,"ProgressionId":7})"),
                 std::runtime_error);
}
