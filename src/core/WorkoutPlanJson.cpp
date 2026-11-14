#include "core/WorkoutPlan.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace workout::core {
namespace {

using json = nlohmann::json;

constexpr int kMinSets = 1;
constexpr int kMaxSets = 20;
constexpr int kMinReps = 1;
constexpr int kMaxReps = 100;
constexpr int kMinRestSec = 0;
constexpr int kMaxRestSec = 3600;

bool isBlank(std::string_view text) noexcept {
    return std::all_of(text.begin(), text.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
}

PlannedExercise plannedExerciseFromJson(const json& value) {
    if (!value.is_object()) {
        throw std::runtime_error("Exercise must be a JSON object");
    }

    PlannedExercise exercise;
    exercise.exerciseId = value.at("ExerciseId").get<ExerciseId>();
    exercise.sets = value.at("Sets").get<int>();
    exercise.reps = value.at("Reps").get<int>();
    exercise.restSec = value.at("RestSec").get<int>();

    if (exercise.exerciseId <= 0) {
        throw std::runtime_error("ExerciseId must be positive");
    }
    if (exercise.sets < kMinSets || exercise.sets > kMaxSets) {
        throw std::runtime_error("Sets must be between 1 and 20");
    }
    if (exercise.reps < kMinReps || exercise.reps > kMaxReps) {
        throw std::runtime_error("Reps must be between 1 and 100");
    }
    if (exercise.restSec < kMinRestSec || exercise.restSec > kMaxRestSec) {
        throw std::runtime_error("RestSec must be between 0 and 3600");
    }

    return exercise;
}

PlanDay planDayFromJson(const json& value) {
    if (!value.is_object()) {
        throw std::runtime_error("Day must be a JSON object");
    }

    PlanDay day;
    day.dayIndex = value.at("DayIndex").get<int>();
    day.name = value.at("Name").get<std::string>();

    if (day.dayIndex < 1) {
        throw std::runtime_error("DayIndex must be positive");
    }
    if (isBlank(day.name)) {
        throw std::runtime_error("Name must not be empty or whitespace-only");
    }

    const json& exercises = value.at("Exercises");
    if (!exercises.is_array()) {
        throw std::runtime_error("Exercises must be an array");
    }

    day.exercises.reserve(exercises.size());
    for (const auto& exercise : exercises) {
        day.exercises.push_back(plannedExerciseFromJson(exercise));
    }

    return day;
}

} // namespace

std::string WorkoutPlan::toJson() const {
    json root;
    root["Days"] = json::array();

    for (const PlanDay& day : days_) {
        json dayJson;
        dayJson["DayIndex"] = day.dayIndex;
        dayJson["Name"] = day.name;
        dayJson["Exercises"] = json::array();

        for (const PlannedExercise& exercise : day.exercises) {
            dayJson["Exercises"].push_back({
                {"ExerciseId", exercise.exerciseId},
                {"Sets", exercise.sets},
                {"Reps", exercise.reps},
                {"RestSec", exercise.restSec}
            });
        }

        root["Days"].push_back(std::move(dayJson));
    }

    return root.dump(2);
}

WorkoutPlan WorkoutPlan::fromJson(const std::string& text) {
    try {
        const json root = json::parse(text);
        if (!root.is_object()) {
            throw std::runtime_error("Root must be a JSON object");
        }

        const json& daysJson = root.at("Days");
        if (!daysJson.is_array()) {
            throw std::runtime_error("Days must be an array");
        }

        std::vector<PlanDay> days;
        days.reserve(daysJson.size());

        std::set<int> dayIndexes;
        for (const auto& dayJson : daysJson) {
            PlanDay day = planDayFromJson(dayJson);
            if (!dayIndexes.insert(day.dayIndex).second) {
                throw std::runtime_error("DayIndex values must be unique");
            }
            days.push_back(std::move(day));
        }

        return WorkoutPlan{std::move(days)};
    } catch (const std::exception& error) {
        throw std::runtime_error(std::string("WorkoutPlan::fromJson: ") + error.what());
    }
}

} // namespace workout::core
