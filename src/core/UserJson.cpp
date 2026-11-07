#include "core/User.h"

#include "EquipmentNames.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace workout::core {
namespace {

using json = nlohmann::json;

struct NamedConstraint {
    HealthConstraint value;
    const char* name;
};

constexpr std::array<NamedConstraint, 9> kConstraintNames{{
    {HealthConstraint::LowerBackPain, "LowerBackPain"},
    {HealthConstraint::KneePain, "KneePain"},
    {HealthConstraint::ShoulderImpingement, "ShoulderImpingement"},
    {HealthConstraint::ElbowTendinopathy, "ElbowTendinopathy"},
    {HealthConstraint::WristPain, "WristPain"},
    {HealthConstraint::HipImpingement, "HipImpingement"},
    {HealthConstraint::CardiovascularIssue, "CardiovascularIssue"},
    {HealthConstraint::Hernia, "Hernia"},
    {HealthConstraint::Custom, "Custom"}
}};

const char* genderToString(Gender gender) noexcept {
    switch (gender) {
        case Gender::Male:   return "Male";
        case Gender::Female: return "Female";
        case Gender::Other:  return "Other";
    }
    return "Unknown";
}

const char* levelToString(ExperienceLevel level) noexcept {
    switch (level) {
        case ExperienceLevel::Novice:       return "Novice";
        case ExperienceLevel::Intermediate: return "Intermediate";
        case ExperienceLevel::Advanced:     return "Advanced";
    }
    return "Unknown";
}

const char* goalToString(TrainingGoal goal) noexcept {
    switch (goal) {
        case TrainingGoal::Strength:    return "Strength";
        case TrainingGoal::Hypertrophy: return "Hypertrophy";
        case TrainingGoal::WeightLoss:  return "WeightLoss";
        case TrainingGoal::Maintenance: return "Maintenance";
    }
    return "Unknown";
}

const char* frequencyToString(TrainingFrequency frequency) noexcept {
    switch (frequency) {
        case TrainingFrequency::Days2: return "Days2";
        case TrainingFrequency::Days3: return "Days3";
        case TrainingFrequency::Days4: return "Days4";
        case TrainingFrequency::Days5: return "Days5";
        case TrainingFrequency::Days6: return "Days6";
    }
    return "Unknown";
}

Gender genderFromString(std::string_view value) {
    if (value == "Male")   return Gender::Male;
    if (value == "Female") return Gender::Female;
    if (value == "Other")  return Gender::Other;
    throw std::runtime_error("Unknown Gender value: " + std::string(value));
}

ExperienceLevel levelFromString(std::string_view value) {
    if (value == "Novice")       return ExperienceLevel::Novice;
    if (value == "Intermediate") return ExperienceLevel::Intermediate;
    if (value == "Advanced")     return ExperienceLevel::Advanced;
    throw std::runtime_error("Unknown ExperienceLevel value: " + std::string(value));
}

TrainingGoal goalFromString(std::string_view value) {
    if (value == "Strength")    return TrainingGoal::Strength;
    if (value == "Hypertrophy") return TrainingGoal::Hypertrophy;
    if (value == "WeightLoss")  return TrainingGoal::WeightLoss;
    if (value == "Maintenance") return TrainingGoal::Maintenance;
    throw std::runtime_error("Unknown TrainingGoal value: " + std::string(value));
}

TrainingFrequency frequencyFromString(std::string_view value) {
    if (value == "Days2") return TrainingFrequency::Days2;
    if (value == "Days3") return TrainingFrequency::Days3;
    if (value == "Days4") return TrainingFrequency::Days4;
    if (value == "Days5") return TrainingFrequency::Days5;
    if (value == "Days6") return TrainingFrequency::Days6;
    throw std::runtime_error("Unknown TrainingFrequency value: " + std::string(value));
}

HealthConstraint constraintFromString(std::string_view value) {
    for (const auto& entry : kConstraintNames) {
        if (value == entry.name) {
            return entry.value;
        }
    }
    throw std::runtime_error("Unknown HealthConstraint value: " + std::string(value));
}

std::vector<std::string> constraintsToStrings(uint16_t mask) {
    std::vector<std::string> result;
    for (const auto& entry : kConstraintNames) {
        if ((mask & HC_MASK(entry.value)) != 0) {
            result.emplace_back(entry.name);
        }
    }
    return result;
}

uint16_t constraintsFromJson(const json& value) {
    if (!value.is_array()) {
        throw std::runtime_error("Constraints must be an array of strings");
    }

    uint16_t mask = 0;
    for (const auto& item : value) {
        if (!item.is_string()) {
            throw std::runtime_error("Constraints must be an array of strings");
        }
        const std::string name = item.get<std::string>();
        if (name == "None") {
            continue;
        }
        mask |= HC_MASK(constraintFromString(name));
    }
    return mask;
}

bool boolValue(const json& object, const char* key, bool fallback) {
    if (!object.contains(key) || object[key].is_null()) {
        return fallback;
    }
    return object[key].get<bool>();
}

std::string stringValue(const json& object, const char* key, std::string fallback) {
    if (!object.contains(key) || object[key].is_null()) {
        return fallback;
    }
    return object[key].get<std::string>();
}

std::string toIso8601(std::chrono::system_clock::time_point timePoint) {
    const std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif

    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

std::chrono::system_clock::time_point fromIso8601(const std::string& text) {
    std::tm utc{};
    std::istringstream in(text);
    in.imbue(std::locale::classic());
    in >> std::get_time(&utc, "%Y-%m-%dT%H:%M:%SZ");

    if (in.fail() || in.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("Invalid ISO 8601 timestamp: " + text);
    }

#if defined(_WIN32)
    const std::time_t time = _mkgmtime(&utc);
#else
    const std::time_t time = timegm(&utc);
#endif

    if (time == static_cast<std::time_t>(-1)) {
        throw std::runtime_error("Invalid ISO 8601 timestamp: " + text);
    }

    const auto parsed = std::chrono::system_clock::from_time_t(time);
    if (toIso8601(parsed) != text) {
        throw std::runtime_error("Invalid ISO 8601 timestamp: " + text);
    }

    return parsed;
}

} // namespace

std::string UserProfile::toJson() const {
    json root;

    root["Id"] = id_;
    root["Name"] = name_;
    root["Gender"] = genderToString(gender_);
    root["Level"] = levelToString(level_);
    root["Goal"] = goalToString(goal_);
    root["Frequency"] = frequencyToString(frequency_);

    root["BodyMetrics"] = {
        {"HeightCm", bodyMetrics_.heightCm},
        {"WeightKg", bodyMetrics_.weightKg},
        {"BodyFatPct", bodyMetrics_.bodyFatPct},
        {"AgeYears", bodyMetrics_.ageYears},
        {"TrainingAgeMonths", bodyMetrics_.trainingAgeMonths},
        {"Bmi", bodyMetrics_.bmi},
        {"BmrKcal", bodyMetrics_.bmrKcal},
        {"TdeeKcal", bodyMetrics_.tdeeKcal},
        {"Ffmi", bodyMetrics_.ffmi}
    };

    root["Constraints"] = constraintsToStrings(constraints_.constraintsMask);
    root["Equipment"] = detail::equipmentToStrings(constraints_.availableEquipment);

    root["Preferences"] = {
        {"AvoidAxialLoading", constraints_.userAvoidAxialLoading},
        {"AvoidSpinalFlexion", constraints_.userAvoidSpinalFlexion},
        {"AvoidOverheadMotion", constraints_.userAvoidOverheadMotion},
        {"AvoidDeepKneeFlexion", constraints_.userAvoidDeepKneeFlexion},
        {"PreferUnilateral", constraints_.userPreferUnilateral},
        {"MaxSessionMinutes", constraints_.maxSessionMinutes
                                  ? json(*constraints_.maxSessionMinutes)
                                  : json(nullptr)},
        {"CustomNotes", constraints_.customNotes}
    };

    root["Estimated1RM"] = json::array();
    for (const auto& [exerciseId, kg] : estimated1RM_) {
        root["Estimated1RM"].push_back({
            {"ExerciseId", exerciseId},
            {"Kg", kg}
        });
    }

    root["CreatedAt"] = toIso8601(createdAt_);
    root["UpdatedAt"] = toIso8601(updatedAt_);

    return root.dump(2);
}

UserProfile UserProfile::fromJson(const std::string& text) {
    try {
        const json root = json::parse(text);
        if (!root.is_object()) {
            throw std::runtime_error("Root must be a JSON object");
        }

        const std::string name = root.at("Name").get<std::string>();
        const Gender gender = genderFromString(root.at("Gender").get<std::string>());
        const ExperienceLevel level = levelFromString(root.at("Level").get<std::string>());
        const TrainingGoal goal = goalFromString(root.at("Goal").get<std::string>());
        const TrainingFrequency frequency =
            frequencyFromString(root.at("Frequency").get<std::string>());

        const json& body = root.at("BodyMetrics");
        if (!body.is_object()) {
            throw std::runtime_error("BodyMetrics must be a JSON object");
        }

        BodyMetrics metrics;
        metrics.heightCm = body.at("HeightCm").get<double>();
        metrics.weightKg = body.at("WeightKg").get<double>();
        metrics.ageYears = body.at("AgeYears").get<int>();
        metrics.bodyFatPct =
            (body.contains("BodyFatPct") && !body["BodyFatPct"].is_null())
                ? body["BodyFatPct"].get<double>()
                : -1.0;
        metrics.trainingAgeMonths =
            (body.contains("TrainingAgeMonths") && !body["TrainingAgeMonths"].is_null())
                ? body["TrainingAgeMonths"].get<int>()
                : 0;

        UserConstraints constraints;
        if (root.contains("Constraints") && !root["Constraints"].is_null()) {
            constraints.constraintsMask = constraintsFromJson(root["Constraints"]);
        }
        if (root.contains("Equipment") && !root["Equipment"].is_null()) {
            constraints.availableEquipment = detail::equipmentFromJson(root["Equipment"]);
        }
        if (root.contains("Preferences") && !root["Preferences"].is_null()) {
            const json& prefs = root["Preferences"];
            if (!prefs.is_object()) {
                throw std::runtime_error("Preferences must be a JSON object");
            }
            constraints.userAvoidAxialLoading = boolValue(prefs, "AvoidAxialLoading", false);
            constraints.userAvoidSpinalFlexion = boolValue(prefs, "AvoidSpinalFlexion", false);
            constraints.userAvoidOverheadMotion = boolValue(prefs, "AvoidOverheadMotion", false);
            constraints.userAvoidDeepKneeFlexion =
                boolValue(prefs, "AvoidDeepKneeFlexion", false);
            constraints.userPreferUnilateral = boolValue(prefs, "PreferUnilateral", false);
            constraints.customNotes = stringValue(prefs, "CustomNotes", {});
            if (prefs.contains("MaxSessionMinutes") && !prefs["MaxSessionMinutes"].is_null()) {
                constraints.maxSessionMinutes = prefs["MaxSessionMinutes"].get<int>();
            }
        }

        const ValidationResult validation = validate(name,
                                                     metrics.ageYears,
                                                     gender,
                                                     metrics.heightCm,
                                                     metrics.weightKg,
                                                     level,
                                                     goal,
                                                     frequency,
                                                     constraints,
                                                     metrics.bodyFatPct,
                                                     metrics.trainingAgeMonths);
        if (!validation.isValid) {
            throw std::runtime_error("Invalid profile: " + validation.toString());
        }

        metrics.recalculateDerived(gender, frequency, level);

        int64_t id = -1;
        if (root.contains("Id") && !root["Id"].is_null()) {
            id = root["Id"].get<int64_t>();
        }
        if (id < -1) {
            throw std::runtime_error("Id must be -1 (unsaved) or non-negative");
        }

        const auto now = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point createdAt = now;
        if (root.contains("CreatedAt") && !root["CreatedAt"].is_null()) {
            createdAt = fromIso8601(root["CreatedAt"].get<std::string>());
        }

        std::chrono::system_clock::time_point updatedAt = createdAt;
        if (root.contains("UpdatedAt") && !root["UpdatedAt"].is_null()) {
            updatedAt = fromIso8601(root["UpdatedAt"].get<std::string>());
        }
        if (updatedAt < createdAt) {
            throw std::runtime_error("UpdatedAt must not be earlier than CreatedAt");
        }

        UserProfile profile(id,
                            name,
                            gender,
                            metrics,
                            level,
                            goal,
                            frequency,
                            constraints,
                            createdAt,
                            updatedAt);

        if (root.contains("Estimated1RM") && !root["Estimated1RM"].is_null()) {
            const json& records = root["Estimated1RM"];
            if (!records.is_array()) {
                throw std::runtime_error("Estimated1RM must be an array");
            }

            for (const auto& record : records) {
                const ExerciseId exerciseId = record.at("ExerciseId").get<ExerciseId>();
                const double kg = record.at("Kg").get<double>();
                if (exerciseId <= 0) {
                    throw std::runtime_error("Estimated1RM exercise id must be positive");
                }
                if (!std::isfinite(kg) || kg <= 0.0) {
                    throw std::runtime_error("Estimated1RM weight must be positive and finite");
                }
                profile.estimated1RM_[exerciseId] = kg;
            }
        }

        return profile;
    } catch (const std::exception& error) {
        throw std::runtime_error(std::string("UserProfile::fromJson: ") + error.what());
    }
}

} // namespace workout::core
