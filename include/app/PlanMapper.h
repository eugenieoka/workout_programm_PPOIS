#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "app/PlanInfo.h"
#include "core/WorkoutPlan.h"
#include "db/PlanRepository.h"

namespace workout::app {

/**
 * @brief Converts between the domain plan and the database DTO layer.
 *
 * The mapper always writes explicit zero-based dayIndex and position values.
 * PlanGenerator may still use one-based indices until the core dayIndex fix
 * lands; the vector order is authoritative here.
 */
class PlanMapper {
public:
    /**
     * @brief Build a persistable PlanRecord from a generated WorkoutPlan.
     *
     * @throws std::invalid_argument if userId, name, or plan contents are invalid.
     */
    [[nodiscard]] static infra::PlanRecord fromGeneratedPlan(
        const core::WorkoutPlan& plan,
        int64_t userId,
        const std::string& name,
        core::TrainingGoal goal,
        int daysPerWeek,
        bool isActive);

    /**
     * @brief Build a presentation model with exercise names already resolved.
     *
     * Missing exercise names fall back to "#<id>" so the UI never has to query
     * the catalog per exercise.
     */
    [[nodiscard]] static PlanInfo toPlanInfo(
        const infra::PlanRecord& record,
        const std::map<core::ExerciseId, std::string>& exerciseNames);
};

} // namespace workout::app
