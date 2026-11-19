#include "core/WorkoutPlan.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace workout::core {
namespace {

using json = nlohmann::json;

PlannedExercise plannedExerciseFromJson(const json& value) {
    if (!value.is_object()) {
        throw std::runtime_error("Exercise must be a JSON object");
    }

    PlannedExercise exercise;
    exercise.exerciseId = value.at("ExerciseId").get<ExerciseId>();
    exercise.sets = value.at("Sets").get<int>();
    exercise.reps = value.at("Reps").get<int>();
    exercise.restSec = value.at("RestSec").get<int>();
    return exercise;
}

PlanDay planDayFromJson(const json& value) {
    if (!value.is_object()) {
        throw std::runtime_error("Day must be a JSON object");
    }

    PlanDay day;
    day.dayIndex = value.at("DayIndex").get<int>();
    day.name = value.at("Name").get<std::string>();

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
    const ValidationResult validation = validate();
    if (!validation.isValid) {
        throw std::invalid_argument(validation.toString());
    }

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
        for (const auto& dayJson : daysJson) {
            days.push_back(planDayFromJson(dayJson));
        }

        WorkoutPlan plan{std::move(days)};
        const ValidationResult validation = plan.validate();
        if (!validation.isValid) {
            throw std::runtime_error(validation.toString());
        }
        return plan;
    } catch (const std::exception& error) {
        throw std::runtime_error(std::string("WorkoutPlan::fromJson: ") + error.what());
    }
}

} // namespace workout::core
