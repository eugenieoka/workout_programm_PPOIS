#include "core/User.h"

#include <gtest/gtest.h>

#include <regex>
#include <stdexcept>
#include <string>

namespace {

using namespace workout::core;

UserProfile makeProfile() {
    UserConstraints constraints;
    constraints.availableEquipment = Equipment::Barbell | Equipment::Dumbbells;
    constraints.addConstraint(HealthConstraint::LowerBackPain);
    constraints.userAvoidOverheadMotion = true;
    constraints.maxSessionMinutes = 60;
    constraints.customNotes = "notes";

    auto profile = UserProfile::create("Alice",
                                       30,
                                       Gender::Female,
                                       170.0,
                                       65.0,
                                       ExperienceLevel::Intermediate,
                                       TrainingGoal::Hypertrophy,
                                       TrainingFrequency::Days4,
                                       constraints,
                                       20.0,
                                       24);
    EXPECT_TRUE(profile.has_value());
    return profile->withEstimated1RM(10, 100.0).withEstimated1RM(20, 80.0);
}

std::string replaceOnce(std::string text,
                        const std::string& from,
                        const std::string& to) {
    const auto position = text.find(from);
    if (position != std::string::npos) {
        text.replace(position, from.size(), to);
    }
    return text;
}

std::string replaceJsonStringValue(std::string text,
                                   const std::string& key,
                                   const std::string& value) {
    const std::string token = "\"" + key + "\": \"";
    const auto position = text.find(token);
    if (position == std::string::npos) {
        return text;
    }

    const auto valueStart = position + token.size();
    const auto valueEnd = text.find('"', valueStart);
    if (valueEnd == std::string::npos) {
        return text;
    }

    text.replace(valueStart, valueEnd - valueStart, value);
    return text;
}

} // namespace

TEST(UserJson, SerializesAllFields) {
    const auto profile = makeProfile();
    const std::string text = profile.toJson();

    EXPECT_NE(text.find("\"Name\": \"Alice\""), std::string::npos);
    EXPECT_NE(text.find("\"Gender\": \"Female\""), std::string::npos);
    EXPECT_NE(text.find("\"Level\": \"Intermediate\""), std::string::npos);
    EXPECT_NE(text.find("\"Goal\": \"Hypertrophy\""), std::string::npos);
    EXPECT_NE(text.find("\"Frequency\": \"Days4\""), std::string::npos);
    EXPECT_NE(text.find("\"BodyFatPct\": 20.0"), std::string::npos);
    EXPECT_NE(text.find("\"TrainingAgeMonths\": 24"), std::string::npos);
    EXPECT_NE(text.find("\"LowerBackPain\""), std::string::npos);
    EXPECT_NE(text.find("\"Barbell\""), std::string::npos);
    EXPECT_NE(text.find("\"Dumbbells\""), std::string::npos);
    EXPECT_NE(text.find("\"AvoidOverheadMotion\": true"), std::string::npos);
    EXPECT_NE(text.find("\"MaxSessionMinutes\": 60"), std::string::npos);
    EXPECT_NE(text.find("\"ExerciseId\": 10"), std::string::npos);
    EXPECT_NE(text.find("\"Kg\": 100.0"), std::string::npos);

    const std::regex timestamp(
        R"("CreatedAt": "\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z")");
    EXPECT_TRUE(std::regex_search(text, timestamp));
}

TEST(UserJson, RoundTrips) {
    const auto profile = makeProfile();
    const std::string text = profile.toJson();

    const UserProfile restored = UserProfile::fromJson(text);

    EXPECT_EQ(restored.getId(), profile.getId());
    EXPECT_EQ(restored.getName(), profile.getName());
    EXPECT_EQ(restored.getGender(), profile.getGender());
    EXPECT_EQ(restored.getAge(), profile.getAge());
    EXPECT_EQ(restored.getTrainingAgeMonths(), profile.getTrainingAgeMonths());
    EXPECT_EQ(restored.getLevel(), profile.getLevel());
    EXPECT_EQ(restored.getGoal(), profile.getGoal());
    EXPECT_EQ(restored.getFrequency(), profile.getFrequency());

    EXPECT_DOUBLE_EQ(restored.getBodyMetrics().heightCm,
                     profile.getBodyMetrics().heightCm);
    EXPECT_DOUBLE_EQ(restored.getBodyMetrics().weightKg,
                     profile.getBodyMetrics().weightKg);
    EXPECT_DOUBLE_EQ(restored.getBodyMetrics().bodyFatPct,
                     profile.getBodyMetrics().bodyFatPct);
    EXPECT_DOUBLE_EQ(restored.getBodyMetrics().bmi, profile.getBodyMetrics().bmi);
    EXPECT_DOUBLE_EQ(restored.getBodyMetrics().bmrKcal,
                     profile.getBodyMetrics().bmrKcal);
    EXPECT_DOUBLE_EQ(restored.getBodyMetrics().tdeeKcal,
                     profile.getBodyMetrics().tdeeKcal);
    EXPECT_DOUBLE_EQ(restored.getBodyMetrics().ffmi, profile.getBodyMetrics().ffmi);

    EXPECT_EQ(restored.getConstraints().constraintsMask,
              profile.getConstraints().constraintsMask);
    EXPECT_EQ(restored.getConstraints().availableEquipment,
              profile.getConstraints().availableEquipment);
    EXPECT_TRUE(restored.getConstraints().userAvoidOverheadMotion);
    ASSERT_TRUE(restored.getConstraints().maxSessionMinutes.has_value());
    EXPECT_EQ(*restored.getConstraints().maxSessionMinutes, 60);
    EXPECT_EQ(restored.getConstraints().customNotes, "notes");

    EXPECT_DOUBLE_EQ(restored.getEstimated1RM(10), 100.0);
    EXPECT_DOUBLE_EQ(restored.getEstimated1RM(20), 80.0);

    EXPECT_EQ(restored.toJson(), text);
}

TEST(UserJson, RejectsMalformedJson) {
    EXPECT_THROW(UserProfile::fromJson("{"), std::runtime_error);
    EXPECT_THROW(UserProfile::fromJson("[]"), std::runtime_error);
    EXPECT_THROW(UserProfile::fromJson(R"({"Name":"Alice"})"), std::runtime_error);
}

TEST(UserJson, RejectsUnknownEnumValues) {
    const std::string text = makeProfile().toJson();

    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"Gender\": \"Female\"", "\"Gender\": \"Alien\"")),
                 std::runtime_error);
    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"Level\": \"Intermediate\"", "\"Level\": \"Alien\"")),
                 std::runtime_error);
    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"Goal\": \"Hypertrophy\"", "\"Goal\": \"Alien\"")),
                 std::runtime_error);
    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"Frequency\": \"Days4\"", "\"Frequency\": \"Days9\"")),
                 std::runtime_error);
}

TEST(UserJson, RejectsUnknownEquipmentAndConstraints) {
    const std::string text = makeProfile().toJson();

    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"Barbell\"", "\"Nope\"")),
                 std::runtime_error);
    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"LowerBackPain\"", "\"Nope\"")),
                 std::runtime_error);
}

TEST(UserJson, RejectsInvalidBodyMetrics) {
    const std::string text = makeProfile().toJson();

    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"HeightCm\": 170.0", "\"HeightCm\": 10.0")),
                 std::runtime_error);
    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"AgeYears\": 30", "\"AgeYears\": 12")),
                 std::runtime_error);
    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"BodyFatPct\": 20.0", "\"BodyFatPct\": 60.0")),
                 std::runtime_error);
}

TEST(UserJson, RejectsInvalidPreferences) {
    const std::string text = makeProfile().toJson();

    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"AvoidOverheadMotion\": true",
                     "\"AvoidOverheadMotion\": \"yes\"")),
                 std::runtime_error);
    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"MaxSessionMinutes\": 60",
                     "\"MaxSessionMinutes\": 5")),
                 std::runtime_error);
}

TEST(UserJson, RejectsInvalidId) {
    const std::string text = makeProfile().toJson();

    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"Id\": -1", "\"Id\": -2")),
                 std::runtime_error);
}

TEST(UserJson, RejectsInvalidEstimated1RM) {
    const std::string text = makeProfile().toJson();

    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"ExerciseId\": 10", "\"ExerciseId\": 0")),
                 std::runtime_error);
    EXPECT_THROW(UserProfile::fromJson(replaceOnce(
                     text, "\"Kg\": 100.0", "\"Kg\": 0.0")),
                 std::runtime_error);
}

TEST(UserJson, RejectsInvalidTimestamps) {
    const std::string text = makeProfile().toJson();

    EXPECT_THROW(UserProfile::fromJson(replaceJsonStringValue(
                     text, "CreatedAt", "not-a-date")),
                 std::runtime_error);
    EXPECT_THROW(UserProfile::fromJson(replaceJsonStringValue(
                     text, "UpdatedAt", "2000-01-01T00:00:00Z")),
                 std::runtime_error);
}
