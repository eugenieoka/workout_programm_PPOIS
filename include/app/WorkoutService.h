#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "app/ExerciseFilter.h"
#include "app/PlanInfo.h"
#include "core/Exercise.h"
#include "core/User.h"
#include "core/WorkoutPlan.h"
#include "db/Database.h"
#include "db/ExerciseRepository.h"
#include "db/PlanRepository.h"
#include "db/UserRepository.h"

namespace workout::app {

/**
 * @brief UI-agnostic application service.
 *
 * The service never reads from stdin or writes to stdout. It exposes domain
 * objects and PlanInfo, and reports recoverable failures with ServiceError
 * codes or std::optional/bool return values.
 */
class WorkoutService {
public:
    explicit WorkoutService(infra::Database& db);

    // Profiles
    [[nodiscard]] std::vector<core::UserProfile> listProfiles();
    [[nodiscard]] std::optional<core::UserProfile> getProfile(int64_t id);
    int64_t saveProfile(const core::UserProfile& profile);
    bool deleteProfile(int64_t id);

    // One-rep maxes
    [[nodiscard]] std::map<core::ExerciseId, double> getOneRepMaxes(int64_t userId);
    void setOneRepMax(int64_t userId, core::ExerciseId exerciseId, double kg);
    void removeOneRepMax(int64_t userId, core::ExerciseId exerciseId);

    // Catalog
    [[nodiscard]] std::vector<core::Exercise> listExercises(
        const ExerciseFilter& filter = {});
    [[nodiscard]] std::optional<core::Exercise> getExercise(core::ExerciseId id);
    [[nodiscard]] std::vector<core::Exercise> findCompatible(
        const core::UserConstraints& constraints);

    // Generation
    [[nodiscard]] core::WorkoutPlan generatePlan(const core::UserProfile& profile);

    // Plans
    [[nodiscard]] std::vector<PlanInfo> listPlans(int64_t userId);
    [[nodiscard]] std::optional<PlanInfo> getPlan(int64_t planId);
    int64_t saveGeneratedPlan(const core::UserProfile& profile,
                              const core::WorkoutPlan& plan,
                              const std::string& name,
                              bool isActive);
    bool updatePlanExercise(int64_t planId, int dayIndex, int position,
                            int sets, int reps, int restSec,
                            std::optional<double> weightKg);
    bool activatePlan(int64_t planId);
    bool deletePlan(int64_t planId);

private:
    [[nodiscard]] std::map<core::ExerciseId, std::string> exerciseNames();
    [[nodiscard]] core::UserProfile requireProfile(int64_t userId);

    infra::Database& db_;
    infra::UserRepository users_;
    infra::ExerciseRepository exercises_;
    infra::PlanRepository plans_;
};

} // namespace workout::app
