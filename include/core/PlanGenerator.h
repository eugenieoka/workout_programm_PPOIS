#pragma once

#include <vector>

#include "Exercise.h"
#include "User.h"
#include "WorkoutPlan.h"

namespace workout::core {

class PlanGenerator {
public:
    /**
     * @brief Build a weekly workout plan from a user profile and exercise catalog.
     *
     * Exercises are filtered by compatibility with the user's constraints and
     * by positive exercise id. The result is deterministic for the same input.
     *
     * @return Empty plan when the catalog has no eligible exercises.
     */
    [[nodiscard]] static WorkoutPlan generate(
        const UserProfile& user,
        const std::vector<Exercise>& catalog);
};

} // namespace workout::core
