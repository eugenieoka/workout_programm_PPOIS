#include "core/User.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>

namespace {

using namespace workout::core;

std::chrono::system_clock::time_point now() {
    return std::chrono::system_clock::now();
}

UserProfile makeProfile() {
    auto profile = UserProfile::create("Alice",
                                       30,
                                       Gender::Female,
                                       170.0,
                                       65.0,
                                       ExperienceLevel::Intermediate,
                                       TrainingGoal::Hypertrophy,
                                       TrainingFrequency::Days4);
    EXPECT_TRUE(profile.has_value());
    return *profile;
}

BodyMetrics validMetrics() {
    BodyMetrics metrics;
    metrics.heightCm = 170.0;
    metrics.weightKg = 65.0;
    metrics.ageYears = 30;
    metrics.bodyFatPct = -1.0;
    metrics.trainingAgeMonths = 0;
    return metrics;
}

ValidationResult validateProfile(std::string_view name = "Alice",
                                 int age = 30,
                                 Gender gender = Gender::Female,
                                 double height = 170.0,
                                 double weight = 65.0,
                                 ExperienceLevel level = ExperienceLevel::Intermediate,
                                 TrainingGoal goal = TrainingGoal::Hypertrophy,
                                 TrainingFrequency frequency = TrainingFrequency::Days4,
                                 UserConstraints constraints = {},
                                 double bodyFat = -1.0,
                                 int trainingAge = 0) {
    return UserProfile::validate(name,
                                 age,
                                 gender,
                                 height,
                                 weight,
                                 level,
                                 goal,
                                 frequency,
                                 constraints,
                                 bodyFat,
                                 trainingAge);
}

} // namespace

TEST(UserValidation, AcceptsMinimumAndMaximumValues) {
    UserConstraints minimumConstraints;
    const auto minimum = validateProfile("A",
                                         13,
                                         Gender::Male,
                                         50.1,
                                         20.1,
                                         ExperienceLevel::Novice,
                                         TrainingGoal::Strength,
                                         TrainingFrequency::Days2,
                                         minimumConstraints,
                                         -1.0,
                                         0);
    EXPECT_TRUE(minimum.isValid) << minimum.toString();

    UserConstraints maximumConstraints;
    maximumConstraints.maxSessionMinutes = 300;
    const auto maximum = validateProfile(std::string(100, 'A'),
                                         100,
                                         Gender::Female,
                                         249.9,
                                         299.9,
                                         ExperienceLevel::Advanced,
                                         TrainingGoal::Maintenance,
                                         TrainingFrequency::Days6,
                                         maximumConstraints,
                                         50.0,
                                         1200);
    EXPECT_TRUE(maximum.isValid) << maximum.toString();
}

TEST(UserValidation, RejectsInvalidName) {
    EXPECT_FALSE(validateProfile("").isValid);
    EXPECT_FALSE(validateProfile("   ").isValid);
    EXPECT_FALSE(validateProfile(std::string(101, 'A')).isValid);
}

TEST(UserValidation, RejectsInvalidAge) {
    EXPECT_FALSE(validateProfile("Alice", 12).isValid);
    EXPECT_FALSE(validateProfile("Alice", 101).isValid);
}

TEST(UserValidation, RejectsInvalidHeightAndWeight) {
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 50.0).isValid);
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 250.0).isValid);
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 170.0, 20.0).isValid);
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 170.0, 300.0).isValid);

    EXPECT_FALSE(validateProfile("Alice",
                                 30,
                                 Gender::Female,
                                 std::numeric_limits<double>::quiet_NaN())
                     .isValid);
    EXPECT_FALSE(validateProfile("Alice",
                                 30,
                                 Gender::Female,
                                 170.0,
                                 std::numeric_limits<double>::quiet_NaN())
                     .isValid);
}

TEST(UserValidation, RejectsInvalidBodyFat) {
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 170.0, 65.0,
                                 ExperienceLevel::Intermediate,
                                 TrainingGoal::Hypertrophy,
                                 TrainingFrequency::Days4, {}, 2.9)
                     .isValid);
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 170.0, 65.0,
                                 ExperienceLevel::Intermediate,
                                 TrainingGoal::Hypertrophy,
                                 TrainingFrequency::Days4, {}, 50.1)
                     .isValid);
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 170.0, 65.0,
                                 ExperienceLevel::Intermediate,
                                 TrainingGoal::Hypertrophy,
                                 TrainingFrequency::Days4, {},
                                 std::numeric_limits<double>::quiet_NaN())
                     .isValid);
}

TEST(UserValidation, RejectsInvalidTrainingAge) {
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 170.0, 65.0,
                                 ExperienceLevel::Intermediate,
                                 TrainingGoal::Hypertrophy,
                                 TrainingFrequency::Days4, {}, -1.0, -1)
                     .isValid);
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 170.0, 65.0,
                                 ExperienceLevel::Intermediate,
                                 TrainingGoal::Hypertrophy,
                                 TrainingFrequency::Days4, {}, -1.0, 361)
                     .isValid);
}

TEST(UserValidation, RejectsInvalidEnums) {
    EXPECT_FALSE(validateProfile("Alice", 30, static_cast<Gender>(9)).isValid);
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 170.0, 65.0,
                                 static_cast<ExperienceLevel>(9))
                     .isValid);
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 170.0, 65.0,
                                 ExperienceLevel::Intermediate,
                                 static_cast<TrainingGoal>(9))
                     .isValid);
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 170.0, 65.0,
                                 ExperienceLevel::Intermediate,
                                 TrainingGoal::Hypertrophy,
                                 static_cast<TrainingFrequency>(1))
                     .isValid);
}

TEST(UserValidation, RejectsInvalidMaxSessionMinutes) {
    UserConstraints tooShort;
    tooShort.maxSessionMinutes = 9;
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 170.0, 65.0,
                                 ExperienceLevel::Intermediate,
                                 TrainingGoal::Hypertrophy,
                                 TrainingFrequency::Days4, tooShort)
                     .isValid);

    UserConstraints tooLong;
    tooLong.maxSessionMinutes = 301;
    EXPECT_FALSE(validateProfile("Alice", 30, Gender::Female, 170.0, 65.0,
                                 ExperienceLevel::Intermediate,
                                 TrainingGoal::Hypertrophy,
                                 TrainingFrequency::Days4, tooLong)
                     .isValid);
}

TEST(UserValidation, ToStringListsCodesAndMessages) {
    EXPECT_EQ(ValidationResult{}.toString(), "");

    const auto result = validateProfile("", 12);
    EXPECT_FALSE(result.isValid);
    EXPECT_GE(result.errors.size(), 2u);
    EXPECT_EQ(result.errors.size(), result.errorCodes.size());

    const std::string text = result.toString();
    EXPECT_NE(text.find("INVALID_NAME"), std::string::npos);
    EXPECT_NE(text.find("INVALID_AGE"), std::string::npos);
    EXPECT_NE(text.find('\n'), std::string::npos);
}

TEST(UserCreate, ComputesDerivedMetrics) {
    const auto profile = makeProfile();

    EXPECT_EQ(profile.getId(), -1);
    EXPECT_EQ(profile.getName(), "Alice");
    EXPECT_EQ(profile.getAge(), 30);
    EXPECT_EQ(profile.getTrainingAgeMonths(), 0);
    EXPECT_EQ(profile.getGender(), Gender::Female);
    EXPECT_EQ(profile.getLevel(), ExperienceLevel::Intermediate);
    EXPECT_EQ(profile.getGoal(), TrainingGoal::Hypertrophy);
    EXPECT_EQ(profile.getFrequency(), TrainingFrequency::Days4);

    EXPECT_NEAR(profile.getBodyMetrics().bmi, 65.0 / (1.7 * 1.7), 1e-9);
    EXPECT_NEAR(profile.getBodyMetrics().bmrKcal, 1401.5, 1e-9);
    EXPECT_NEAR(profile.getBodyMetrics().tdeeKcal, 1401.5 * 1.65, 1e-9);
    EXPECT_DOUBLE_EQ(profile.getBodyMetrics().ffmi, 0.0);
}

TEST(UserCreate, ComputesFfmiWhenBodyFatIsKnown) {
    auto profile = UserProfile::create("Alice",
                                       30,
                                       Gender::Female,
                                       170.0,
                                       65.0,
                                       ExperienceLevel::Intermediate,
                                       TrainingGoal::Hypertrophy,
                                       TrainingFrequency::Days4,
                                       {},
                                       20.0,
                                       24);
    ASSERT_TRUE(profile.has_value());

    EXPECT_DOUBLE_EQ(profile->getBodyMetrics().bodyFatPct, 20.0);
    EXPECT_EQ(profile->getTrainingAgeMonths(), 24);
    EXPECT_NEAR(profile->getBodyMetrics().ffmi,
                52.0 / (1.7 * 1.7) + 6.1 * (1.8 - 1.7),
                1e-9);
}

TEST(UserCreate, RejectsInvalidInput) {
    EXPECT_FALSE(UserProfile::create("",
                                     30,
                                     Gender::Female,
                                     170.0,
                                     65.0,
                                     ExperienceLevel::Intermediate,
                                     TrainingGoal::Hypertrophy,
                                     TrainingFrequency::Days4)
                     .has_value());
}

TEST(UserCreateFromDb, AcceptsZeroIdAndRejectsNegativeId) {
    const auto timestamp = now();
    const auto profile = UserProfile::createFromDb(0,
                                                   "Bob",
                                                   Gender::Male,
                                                   validMetrics(),
                                                   ExperienceLevel::Novice,
                                                   TrainingGoal::Strength,
                                                   TrainingFrequency::Days3,
                                                   {},
                                                   timestamp,
                                                   timestamp);

    EXPECT_EQ(profile.getId(), 0);
    EXPECT_EQ(profile.getName(), "Bob");

    EXPECT_THROW(UserProfile::createFromDb(-1,
                                           "Bob",
                                           Gender::Male,
                                           validMetrics(),
                                           ExperienceLevel::Novice,
                                           TrainingGoal::Strength,
                                           TrainingFrequency::Days3,
                                           {},
                                           timestamp,
                                           timestamp),
                 std::invalid_argument);
}

TEST(UserUpdates, WithBodyWeightRecalculatesDerivedMetrics) {
    const auto profile = makeProfile();
    const auto updated = profile.withBodyWeight(70.0);

    EXPECT_DOUBLE_EQ(updated.getBodyMetrics().weightKg, 70.0);
    EXPECT_NEAR(updated.getBodyMetrics().bmi, 70.0 / (1.7 * 1.7), 1e-9);
    EXPECT_NEAR(updated.getBodyMetrics().bmrKcal,
                10.0 * 70.0 + 6.25 * 170.0 - 5.0 * 30.0 - 161.0,
                1e-9);

    EXPECT_THROW(profile.withBodyWeight(10.0), std::invalid_argument);
}

TEST(UserUpdates, WithBodyFatPctRecalculatesFfmi) {
    const auto profile = makeProfile();
    const auto updated = profile.withBodyFatPct(20.0);

    EXPECT_DOUBLE_EQ(updated.getBodyMetrics().bodyFatPct, 20.0);
    EXPECT_NEAR(updated.getBodyMetrics().ffmi,
                52.0 / (1.7 * 1.7) + 6.1 * (1.8 - 1.7),
                1e-9);

    EXPECT_THROW(profile.withBodyFatPct(60.0), std::invalid_argument);
}

TEST(UserUpdates, WithTrainingAgeMonths) {
    const auto profile = makeProfile();

    EXPECT_EQ(profile.withTrainingAgeMonths(24).getTrainingAgeMonths(), 24);
    EXPECT_THROW(profile.withTrainingAgeMonths(5000), std::invalid_argument);
}

TEST(UserUpdates, WithLevelRecalculatesTdee) {
    const auto profile = makeProfile();
    const auto advanced = profile.withLevel(ExperienceLevel::Advanced);

    EXPECT_EQ(advanced.getLevel(), ExperienceLevel::Advanced);
    EXPECT_NEAR(advanced.getBodyMetrics().tdeeKcal, 1401.5 * 1.70, 1e-9);

    EXPECT_THROW(profile.withLevel(static_cast<ExperienceLevel>(9)),
                 std::invalid_argument);
}

TEST(UserUpdates, WithGoalKeepsTdee) {
    const auto profile = makeProfile();
    const auto strength = profile.withGoal(TrainingGoal::Strength);

    EXPECT_EQ(strength.getGoal(), TrainingGoal::Strength);
    EXPECT_DOUBLE_EQ(strength.getBodyMetrics().tdeeKcal,
                     profile.getBodyMetrics().tdeeKcal);

    EXPECT_THROW(profile.withGoal(static_cast<TrainingGoal>(9)),
                 std::invalid_argument);
}

TEST(UserUpdates, WithFrequencyRecalculatesTdee) {
    const auto profile = makeProfile();
    const auto days6 = profile.withFrequency(TrainingFrequency::Days6);

    EXPECT_EQ(days6.getFrequency(), TrainingFrequency::Days6);
    EXPECT_NEAR(days6.getBodyMetrics().tdeeKcal, 1401.5 * 1.80, 1e-9);

    EXPECT_THROW(profile.withFrequency(static_cast<TrainingFrequency>(1)),
                 std::invalid_argument);
}

TEST(UserUpdates, WithConstraints) {
    const auto profile = makeProfile();

    UserConstraints constraints;
    constraints.availableEquipment = Equipment::Barbell;
    constraints.maxSessionMinutes = 45;

    const auto updated = profile.withConstraints(constraints);
    EXPECT_EQ(updated.getConstraints().availableEquipment, Equipment::Barbell);
    ASSERT_TRUE(updated.getConstraints().maxSessionMinutes.has_value());
    EXPECT_EQ(*updated.getConstraints().maxSessionMinutes, 45);

    UserConstraints invalid;
    invalid.maxSessionMinutes = 9;
    EXPECT_THROW(profile.withConstraints(invalid), std::invalid_argument);
}

TEST(UserUpdates, WithName) {
    const auto profile = makeProfile();

    EXPECT_EQ(profile.withName("Bob").getName(), "Bob");
    EXPECT_THROW(profile.withName(""), std::invalid_argument);
    EXPECT_THROW(profile.withName(std::string(101, 'x')), std::invalid_argument);
}

TEST(UserUpdates, WithEstimated1RM) {
    const auto profile = makeProfile();

    const auto withOneRepMax = profile.withEstimated1RM(10, 100.0);
    EXPECT_DOUBLE_EQ(withOneRepMax.getEstimated1RM(10), 100.0);

    const auto cleared = withOneRepMax.withEstimated1RM(10, 0.0);
    EXPECT_DOUBLE_EQ(cleared.getEstimated1RM(10), 0.0);
    EXPECT_DOUBLE_EQ(profile.getEstimated1RM(999), 0.0);

    EXPECT_THROW(profile.withEstimated1RM(0, 100.0), std::invalid_argument);
    EXPECT_THROW(profile.withEstimated1RM(-1, 100.0), std::invalid_argument);
    EXPECT_THROW(profile.withEstimated1RM(10,
                                          std::numeric_limits<double>::infinity()),
                 std::invalid_argument);
    EXPECT_THROW(profile.withEstimated1RM(10,
                                          std::numeric_limits<double>::quiet_NaN()),
                 std::invalid_argument);
}

TEST(UserCalories, AppliesGoalDelta) {
    const auto profile = makeProfile();
    const double tdee = profile.getBodyMetrics().tdeeKcal;

    EXPECT_NEAR(profile.getTargetCalories(), tdee + 300.0, 1e-9);
    EXPECT_NEAR(profile.withGoal(TrainingGoal::Strength).getTargetCalories(),
                tdee + 200.0,
                1e-9);
    EXPECT_NEAR(profile.withGoal(TrainingGoal::WeightLoss).getTargetCalories(),
                tdee - 400.0,
                1e-9);
    EXPECT_NEAR(profile.withGoal(TrainingGoal::Maintenance).getTargetCalories(),
                tdee,
                1e-9);
}

TEST(UserCalories, ReturnsZeroWhenTdeeIsZero) {
    const auto timestamp = now();
    const auto profile = UserProfile::createFromDb(1,
                                                   "Bob",
                                                   Gender::Male,
                                                   BodyMetrics{},
                                                   ExperienceLevel::Novice,
                                                   TrainingGoal::Strength,
                                                   TrainingFrequency::Days3,
                                                   {},
                                                   timestamp,
                                                   timestamp);

    EXPECT_DOUBLE_EQ(profile.getTargetCalories(), 0.0);
}

TEST(UserHelpers, IsNovice) {
    const auto profile = makeProfile();

    EXPECT_FALSE(profile.isNovice());
    EXPECT_TRUE(profile.withLevel(ExperienceLevel::Novice).isNovice());
}
