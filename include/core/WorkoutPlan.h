#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Types.h"

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

private:
    std::vector<PlanDay> days_;
};

} // namespace workout::core
