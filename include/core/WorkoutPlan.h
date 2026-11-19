#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Types.h"
#include "ValidationResult.h"

namespace workout::core {

struct PlannedExercise {
    ExerciseId exerciseId = -1;
    int sets = 0;
    int reps = 0;
    int restSec = 0;
};

struct PlanDay {
    int dayIndex = 0;
    std::string name;
    std::vector<PlannedExercise> exercises;
};

class WorkoutPlan {
public:
    WorkoutPlan() = default;
    explicit WorkoutPlan(std::vector<PlanDay> days);

    [[nodiscard]] const std::vector<PlanDay>& getDays() const noexcept { return days_; }
    [[nodiscard]] bool isEmpty() const noexcept { return days_.empty(); }
    [[nodiscard]] std::size_t getDayCount() const noexcept { return days_.size(); }

    /**
     * @brief Validate non-empty plan contents.
     *
     * An empty plan is valid. For non-empty plans, day indices must be
     * non-negative and unique, day names must not be blank, and every exercise
     * must have a positive id and in-range sets/reps/restSec.
     */
    [[nodiscard]] ValidationResult validate() const;

    /**
     * @return JSON with PascalCase keys and Days array.
     * @throw std::invalid_argument if validate() fails.
     */
    [[nodiscard]] std::string toJson() const;

    /** @throw std::runtime_error on parse/validation error. */
    static WorkoutPlan fromJson(const std::string& json);

private:
    std::vector<PlanDay> days_;
};

} // namespace workout::core
