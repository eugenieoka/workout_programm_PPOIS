#include "core/WorkoutPlan.h"

#include <utility>

namespace workout::core {

WorkoutPlan::WorkoutPlan(std::vector<PlanDay> days)
    : days_(std::move(days)) {}

} // namespace workout::core
