#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core/Types.h"
#include "core/User.h"

namespace workout::app {

/**
 * @brief One exercise in a plan, ready for presentation.
 *
 * position is zero-based within its day.
 */
struct PlanExerciseInfo {
    int position = 0;
    core::ExerciseId exerciseId = -1;
    std::string exerciseName;
    int sets = 0;
    int reps = 0;
    int restSec = 0;
    std::optional<double> weightKg;
    std::string notes;
};

/**
 * @brief One training day in a plan.
 *
 * dayIndex is zero-based at the DB and service boundary. The UI is responsible
 * for showing dayIndex + 1.
 */
struct PlanDayInfo {
    int dayIndex = 0;
    std::string name;
    std::vector<PlanExerciseInfo> exercises;
};

struct PlanInfo {
    int64_t id = -1;
    int64_t userId = -1;
    std::string name;
    core::TrainingGoal goal = core::TrainingGoal::Strength;
    int daysPerWeek = 0;
    bool isActive = false;
    std::string notes;
    std::string createdAt;
    std::string updatedAt;
    std::vector<PlanDayInfo> days;
};

} // namespace workout::app
